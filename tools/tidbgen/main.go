package main

import (
	"bytes"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"
)

var inputFiles = []string{
	"array",
	"bool",
	"class",
	"dir",
	"display",
	"enumerable",
	"false",
	"file",
	"float",
	"gpio",
	"hash",
	"i2_c",
	"integer",
	"kernel",
	"math",
	"nil",
	"object",
	"proc",
	"range",
	"rng",
	"runtime_error",
	"sandbox",
	"sd",
	"sprite",
	"string",
	"symbol",
	"untyped",
	"widget",
	"widget_list",
	"widget_text_view",
}

var classNames = []string{
	"",
	"Object",
	"Integer",
	"Float",
	"String",
	"Symbol",
	"Array",
	"Hash",
	"Range",
	"RNG",
	"Bool",
	"TrueClass",
	"FalseClass",
	"NilClass",
	"Proc",
	"Kernel",
	"Enumerable",
	"Class",
	"Dir",
	"File",
	"Math",
	"GPIO",
	"I2C",
	"Display",
	"SD",
	"Sandbox",
	"Sprite",
	"Widget",
	"WidgetList",
	"WidgetTextView",
	"RuntimeError",
	"Untyped",
}

type stringList []string

func (values *stringList) UnmarshalJSON(data []byte) error {
	var list []string
	if err := json.Unmarshal(data, &list); err == nil {
		*values = list
		return nil
	}

	var value string
	if err := json.Unmarshal(data, &value); err != nil {
		return err
	}

	*values = []string{value}
	return nil
}

type typeValue struct {
	Type stringList `json:"type"`
}

type argument struct {
	Type       stringList `json:"type"`
	IsAsterisk bool       `json:"is_asterisk"`
	IsDefault  bool       `json:"is_default"`
	Key        string     `json:"key"`
}

type method struct {
	Name            string     `json:"name"`
	Arguments       []argument `json:"arguments"`
	ReturnType      typeValue  `json:"return_type"`
	Document        string     `json:"document"`
	BlockParameters stringList `json:"block_parameters"`
}

type classConfig struct {
	Class           string   `json:"class"`
	Extends         []string `json:"extends"`
	InstanceMethods []method `json:"instance_methods"`
	ClassMethods    []method `json:"class_methods"`
}

type typeResult struct {
	classIDs             []uint8
	arrayVariantClassIDs []uint8
	display              string
}

type flatMethod struct {
	method method
	origin string
}

type generatedMethod struct {
	nameOffset                uint16
	signatureOffset           uint16
	documentOffset            uint16
	returnClassID             uint8
	returnArrayVariantClassID uint8
	returnUnionIndex          uint8
	arrayVariantUnionIndex    uint8
	blockParamClassID         uint8
	originClassID             uint8
}

type generatedClass struct {
	nameOffset          uint16
	instanceMethodStart uint16
	instanceMethodCount uint16
	classMethodStart    uint16
	classMethodCount    uint16
}

type stringPool struct {
	data       bytes.Buffer
	offsets    map[string]uint16
	overflowed bool
}

type unionPool struct {
	entries [][4]uint8
	indexes map[string]uint8
}

type generator struct {
	classes       map[string]classConfig
	classIDs      map[string]uint8
	warnedTypes   map[string]bool
	namePool      *stringPool
	signaturePool *stringPool
	documentPool  *stringPool
	unions        *unionPool
}

func newStringPool() *stringPool {
	pool := &stringPool{
		offsets: make(map[string]uint16),
	}
	pool.add("")
	return pool
}

func (pool *stringPool) add(value string) uint16 {
	if offset, found := pool.offsets[value]; found {
		return offset
	}

	if pool.data.Len()+len(value)+1 > 65535 {
		pool.overflowed = true
		return 0
	}

	offset := uint16(pool.data.Len())
	pool.offsets[value] = offset
	pool.data.WriteString(value)
	pool.data.WriteByte(0)
	return offset
}

func newUnionPool() *unionPool {
	return &unionPool{
		entries: [][4]uint8{{}},
		indexes: map[string]uint8{"": 0},
	}
}

func (pool *unionPool) add(classIDs []uint8) (uint8, error) {
	classIDs = uniqueSortedClassIDs(classIDs)
	if len(classIDs) <= 1 {
		return 0, nil
	}
	if len(classIDs) > 4 {
		return 0, fmt.Errorf("union has %d members", len(classIDs))
	}

	parts := make([]string, len(classIDs))
	for index, classID := range classIDs {
		parts[index] = fmt.Sprintf("%d", classID)
	}
	key := strings.Join(parts, ",")

	if unionIndex, found := pool.indexes[key]; found {
		return unionIndex, nil
	}
	if len(pool.entries) > 255 {
		return 0, errors.New("union table exceeds 255 entries")
	}

	var entry [4]uint8
	copy(entry[:], classIDs)
	unionIndex := uint8(len(pool.entries))
	pool.entries = append(pool.entries, entry)
	pool.indexes[key] = unionIndex
	return unionIndex, nil
}

func uniqueSortedClassIDs(classIDs []uint8) []uint8 {
	seen := make(map[uint8]bool)
	result := make([]uint8, 0, len(classIDs))

	for _, classID := range classIDs {
		if classID == 0 || seen[classID] {
			continue
		}
		seen[classID] = true
		result = append(result, classID)
	}

	sort.Slice(result, func(first int, second int) bool {
		return result[first] < result[second]
	})
	return result
}

func main() {
	configDirectory := flag.String("config", "", "input .ti-config directory")
	outputDirectory := flag.String("out", "", "output directory")
	flag.Parse()

	if *configDirectory == "" || *outputDirectory == "" {
		fmt.Fprintln(os.Stderr, "usage: tidbgen -config DIR -out DIR")
		os.Exit(2)
	}

	if err := run(*configDirectory, *outputDirectory); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}

func run(configDirectory string, outputDirectory string) error {
	classes, err := loadClasses(configDirectory)
	if err != nil {
		return err
	}

	classIDs := make(map[string]uint8)
	for classID, className := range classNames {
		if className != "" {
			classIDs[className] = uint8(classID)
		}
	}

	generator := &generator{
		classes:       classes,
		classIDs:      classIDs,
		warnedTypes:   make(map[string]bool),
		namePool:      newStringPool(),
		signaturePool: newStringPool(),
		documentPool:  newStringPool(),
		unions:        newUnionPool(),
	}

	classesOutput, methodsOutput, err := generator.generate()
	if err != nil {
		return err
	}
	if generator.namePool.overflowed || generator.signaturePool.overflowed ||
		generator.documentPool.overflowed {
		return errors.New("string pool exceeds 64KB")
	}

	header := generateHeader()
	source := generator.generateSource(classesOutput, methodsOutput)

	if err := os.MkdirAll(outputDirectory, 0o755); err != nil {
		return err
	}
	if err := os.WriteFile(
		filepath.Join(outputDirectory, "area512_ti_builtin_db.h"),
		[]byte(header),
		0o644,
	); err != nil {
		return err
	}
	if err := os.WriteFile(
		filepath.Join(outputDirectory, "area512_ti_builtin_db.c"),
		[]byte(source),
		0o644,
	); err != nil {
		return err
	}

	return nil
}

func loadClasses(configDirectory string) (map[string]classConfig, error) {
	classes := make(map[string]classConfig)

	for _, fileName := range inputFiles {
		path := filepath.Join(configDirectory, fileName+".json")
		data, err := os.ReadFile(path)
		if err != nil {
			return nil, err
		}

		var config classConfig
		if err := json.Unmarshal(data, &config); err != nil {
			return nil, fmt.Errorf("%s: %w", path, err)
		}
		if config.Class == "" {
			config.Class = "Object"
		}

		classes[config.Class] = config
	}

	if _, found := classes["TrueClass"]; !found {
		classes["TrueClass"] = classConfig{
			Class:   "TrueClass",
			Extends: []string{"Bool"},
		}
	}

	return classes, nil
}

func (generator *generator) generate() (
	[]generatedClass,
	[]generatedMethod,
	error,
) {
	classesOutput := make([]generatedClass, len(classNames))
	methodsOutput := make([]generatedMethod, 0)

	for classID, className := range classNames {
		classesOutput[classID].nameOffset = generator.namePool.add(className)
		if classID == 0 {
			continue
		}

		instanceMethods := generator.flattenMethods(className, false)
		classesOutput[classID].instanceMethodStart = uint16(len(methodsOutput))
		generated, err := generator.generateMethods(className, instanceMethods)
		if err != nil {
			return nil, nil, err
		}
		methodsOutput = append(methodsOutput, generated...)
		classesOutput[classID].instanceMethodCount = uint16(len(generated))

		classMethods := generator.flattenMethods(className, true)
		classesOutput[classID].classMethodStart = uint16(len(methodsOutput))
		generated, err = generator.generateMethods(className, classMethods)
		if err != nil {
			return nil, nil, err
		}
		methodsOutput = append(methodsOutput, generated...)
		classesOutput[classID].classMethodCount = uint16(len(generated))
	}

	if len(methodsOutput) > 65535 {
		return nil, nil, errors.New("method table exceeds 65535 entries")
	}

	return classesOutput, methodsOutput, nil
}

func (generator *generator) flattenMethods(
	className string,
	useClassMethods bool,
) []flatMethod {
	result := make([]flatMethod, 0)
	seenMethods := make(map[string]bool)
	visitedClasses := make(map[string]bool)

	var appendClass func(string)
	appendClass = func(currentClass string) {
		if (currentClass == "Kernel" && currentClass != className) ||
			visitedClasses[currentClass] {
			return
		}
		visitedClasses[currentClass] = true

		config, found := generator.classes[currentClass]
		if !found {
			return
		}

		methods := config.InstanceMethods
		if useClassMethods {
			methods = config.ClassMethods
		}

		for _, currentMethod := range methods {
			if seenMethods[currentMethod.Name] {
				continue
			}
			seenMethods[currentMethod.Name] = true
			result = append(result, flatMethod{
				method: currentMethod,
				origin: currentClass,
			})
		}

		for _, parent := range config.Extends {
			appendClass(parent)
		}
	}

	appendClass(className)
	if className != "Object" {
		appendClass("Object")
	}

	sort.SliceStable(result, func(first int, second int) bool {
		return result[first].method.Name < result[second].method.Name
	})
	return result
}

func (generator *generator) generateMethods(
	ownerClass string,
	methods []flatMethod,
) ([]generatedMethod, error) {
	result := make([]generatedMethod, 0, len(methods))

	for _, flat := range methods {
		returnType := generator.resolveTypes(
			flat.method.ReturnType.Type,
			ownerClass,
		)
		returnUnionIndex, err := generator.unions.add(returnType.classIDs)
		if err != nil {
			return nil, fmt.Errorf(
				"%s#%s return type: %w",
				ownerClass,
				flat.method.Name,
				err,
			)
		}
		arrayVariantUnionIndex, err := generator.unions.add(returnType.arrayVariantClassIDs)
		if err != nil {
			return nil, fmt.Errorf(
				"%s#%s array variant type: %w",
				ownerClass,
				flat.method.Name,
				err,
			)
		}

		blockParameterClassID := uint8(0)
		if len(flat.method.BlockParameters) > 0 {
			blockType := generator.resolveTypes(
				flat.method.BlockParameters[:1],
				ownerClass,
			)
			if len(blockType.classIDs) > 0 {
				blockParameterClassID = blockType.classIDs[0]
			}
		}

		returnClassID := uint8(0)
		if len(returnType.classIDs) > 0 {
			returnClassID = returnType.classIDs[0]
		}
		returnArrayVariantClassID := uint8(0)
		if len(returnType.arrayVariantClassIDs) > 0 {
			returnArrayVariantClassID = returnType.arrayVariantClassIDs[0]
		}

		originClassID, found := generator.classIDs[flat.origin]
		if !found {
			originClassID = generator.classIDs["Untyped"]
		}

		result = append(result, generatedMethod{
			nameOffset:                generator.namePool.add(flat.method.Name),
			signatureOffset:           generator.signaturePool.add(generator.signature(flat.method, ownerClass)),
			documentOffset:            generator.documentPool.add(flat.method.Document),
			returnClassID:             returnClassID,
			returnArrayVariantClassID: returnArrayVariantClassID,
			returnUnionIndex:          returnUnionIndex,
			arrayVariantUnionIndex:    arrayVariantUnionIndex,
			blockParamClassID:         blockParameterClassID,
			originClassID:             originClassID,
		})
	}

	return result, nil
}

func (generator *generator) signature(currentMethod method, ownerClass string) string {
	arguments := make([]string, 0, len(currentMethod.Arguments))

	for _, currentArgument := range currentMethod.Arguments {
		resolved := generator.resolveTypes(currentArgument.Type, ownerClass)
		display := resolved.display
		if display == "" {
			display = "Untyped"
		}

		splatPrefix := readSplatPrefix(currentArgument.Type)
		if currentArgument.IsAsterisk && splatPrefix == "" {
			splatPrefix = "*"
		}
		display = splatPrefix + display
		if currentArgument.Key != "" {
			display = currentArgument.Key + " " + display
		}
		if currentArgument.IsDefault || hasOptionalPrefix(currentArgument.Type) {
			display += "?"
		}

		arguments = append(arguments, display)
	}

	returnType := generator.resolveTypes(currentMethod.ReturnType.Type, ownerClass)
	return fmt.Sprintf(
		"%s(%s) -> %s",
		currentMethod.Name,
		strings.Join(arguments, ", "),
		returnType.display,
	)
}

func hasOptionalPrefix(values []string) bool {
	for _, value := range values {
		if strings.HasPrefix(strings.TrimSpace(value), "?") {
			return true
		}
	}

	return false
}

func readSplatPrefix(values []string) string {
	result := ""

	for _, value := range values {
		value = strings.TrimSpace(value)
		if strings.HasPrefix(value, "**") {
			return "**"
		}
		if strings.HasPrefix(value, "*") {
			result = "*"
		}
	}

	return result
}

func (generator *generator) resolveTypes(values []string, ownerClass string) typeResult {
	if len(values) == 0 {
		fmt.Fprintln(os.Stderr, "warning: empty type mapped to Untyped")
		return typeResult{
			classIDs: []uint8{generator.classIDs["Untyped"]},
			display:  "Untyped",
		}
	}

	classIDs := make([]uint8, 0)
	arrayVariantClassIDs := make([]uint8, 0)
	displays := make([]string, 0)

	for _, value := range values {
		resolved := generator.resolveType(value, ownerClass)
		classIDs = append(classIDs, resolved.classIDs...)
		arrayVariantClassIDs = append(arrayVariantClassIDs, resolved.arrayVariantClassIDs...)
		displays = append(displays, resolved.display)
	}

	classIDs = uniqueSortedClassIDs(classIDs)
	arrayVariantClassIDs = uniqueSortedClassIDs(arrayVariantClassIDs)
	displays = uniqueStrings(displays)

	return typeResult{
		classIDs:             classIDs,
		arrayVariantClassIDs: arrayVariantClassIDs,
		display:              strings.Join(displays, "|"),
	}
}

func (generator *generator) resolveType(value string, ownerClass string) typeResult {
	value = strings.TrimSpace(value)
	for strings.HasPrefix(value, "?") || strings.HasPrefix(value, "*") {
		value = strings.TrimPrefix(value, "?")
		value = strings.TrimPrefix(value, "*")
	}

	if strings.HasPrefix(value, "[") && strings.HasSuffix(value, "]") {
		element := strings.TrimSpace(value[1 : len(value)-1])
		elementType := generator.resolveType(element, ownerClass)
		return typeResult{
			classIDs:             []uint8{generator.classIDs["Array"]},
			arrayVariantClassIDs: elementType.classIDs,
			display:              "[" + elementType.display + "]",
		}
	}

	if strings.Contains(value, "|") {
		return generator.resolveTypes(strings.Split(value, "|"), ownerClass)
	}

	if value == "Self" || value == "SelfArgument" {
		classID, found := generator.classIDs[ownerClass]
		if !found {
			classID = generator.classIDs["Untyped"]
		}
		return typeResult{classIDs: []uint8{classID}, display: ownerClass}
	}

	aliases := map[string]string{
		"Int":              "Integer",
		"I":                "Integer",
		"DefaultInt":       "Integer",
		"OptionalInt":      "Integer",
		"IntInt":           "Integer",
		"Number":           "Integer",
		"DefaultFloat":     "Float",
		"S":                "String",
		"DefaultString":    "String",
		"DefaultSymbol":    "Symbol",
		"DefaultArray":     "Array",
		"Nil":              "NilClass",
		"IntArray":         "Array",
		"StringArray":      "Array",
		"SelfArray":        "Array",
		"BlockResultArray": "Array",
		"KeyValueArray":    "Array",
	}

	if alias, found := aliases[value]; found {
		if strings.HasSuffix(value, "Array") && value != "DefaultArray" {
			arrayVariantClassName := "Untyped"
			switch value {
			case "IntArray":
				arrayVariantClassName = "Integer"
			case "StringArray":
				arrayVariantClassName = "String"
			case "SelfArray":
				arrayVariantClassName = ownerClass
			}
			return typeResult{
				classIDs:             []uint8{generator.classIDs["Array"]},
				arrayVariantClassIDs: []uint8{generator.classIDs[arrayVariantClassName]},
				display:              "[" + arrayVariantClassName + "]",
			}
		}
		value = alias
	}

	if value == "Untyped" || strings.HasPrefix(value, "Unify") ||
		strings.HasPrefix(value, "Default") || strings.HasPrefix(value, "Optional") {
		return typeResult{
			classIDs: []uint8{generator.classIDs["Untyped"]},
			display:  "Untyped",
		}
	}

	classID, found := generator.classIDs[value]
	if !found {
		generator.warnUnknownType(value)
		return typeResult{
			classIDs: []uint8{generator.classIDs["Untyped"]},
			display:  "Untyped",
		}
	}

	return typeResult{classIDs: []uint8{classID}, display: value}
}

func (generator *generator) warnUnknownType(value string) {
	if generator.warnedTypes == nil {
		generator.warnedTypes = make(map[string]bool)
	}
	if generator.warnedTypes[value] {
		return
	}

	generator.warnedTypes[value] = true
	fmt.Fprintf(os.Stderr, "warning: unknown type %q mapped to Untyped\n", value)
}

func uniqueStrings(values []string) []string {
	seen := make(map[string]bool)
	result := make([]string, 0, len(values))

	for _, value := range values {
		if value == "" || seen[value] {
			continue
		}
		seen[value] = true
		result = append(result, value)
	}

	return result
}

func generateHeader() string {
	return `#ifndef AREA512_TI_BUILTIN_DB_H
#define AREA512_TI_BUILTIN_DB_H

#include <stdint.h>

typedef enum {
  TI_CLASS_NONE = 0,
  TI_CLASS_OBJECT,
  TI_CLASS_INTEGER,
  TI_CLASS_FLOAT,
  TI_CLASS_STRING,
  TI_CLASS_SYMBOL,
  TI_CLASS_ARRAY,
  TI_CLASS_HASH,
  TI_CLASS_RANGE,
  TI_CLASS_RNG,
  TI_CLASS_BOOL,
  TI_CLASS_TRUE,
  TI_CLASS_FALSE,
  TI_CLASS_NIL,
  TI_CLASS_PROC,
  TI_CLASS_KERNEL,
  TI_CLASS_ENUMERABLE,
  TI_CLASS_CLASS,
  TI_CLASS_DIR,
  TI_CLASS_FILE,
  TI_CLASS_MATH,
  TI_CLASS_GPIO,
  TI_CLASS_I2C,
  TI_CLASS_DISPLAY,
  TI_CLASS_SD,
  TI_CLASS_SANDBOX,
  TI_CLASS_SPRITE,
  TI_CLASS_WIDGET,
  TI_CLASS_WIDGET_LIST,
  TI_CLASS_WIDGET_TEXT_VIEW,
  TI_CLASS_RUNTIME_ERROR,
  TI_CLASS_UNTYPED,
  TI_CLASS_BUILTIN_COUNT,
  TI_CLASS_USER_BASE = TI_CLASS_BUILTIN_COUNT
} TiClassId;

typedef struct {
  uint16_t name_offset;
  uint16_t signature_offset;
  uint16_t document_offset;
  uint8_t return_class_id;
  uint8_t return_array_variant_class_id;
  uint8_t return_union_index;
  uint8_t array_variant_union_index;
  uint8_t block_parameter_class_id;
  uint8_t origin_class_id;
} TiBuiltinMethod;

typedef struct {
  uint16_t name_offset;
  uint16_t instance_method_start;
  uint16_t instance_method_count;
  uint16_t static_method_start;
  uint16_t static_method_count;
} TiBuiltinClass;

typedef struct {
  uint8_t member_class_ids[4];
} TiBuiltinUnion;

extern const TiBuiltinClass ti_builtin_classes[];
extern const TiBuiltinMethod ti_builtin_methods[];
extern const TiBuiltinUnion ti_builtin_unions[];
extern const char ti_builtin_name_pool[];
extern const char ti_builtin_signature_pool[];
extern const char ti_builtin_document_pool[];
extern const uint16_t ti_builtin_class_count;
extern const uint16_t ti_builtin_method_count;
extern const uint16_t ti_builtin_union_count;
extern const uint16_t ti_builtin_name_pool_size;
extern const uint16_t ti_builtin_signature_pool_size;
extern const uint16_t ti_builtin_document_pool_size;

#endif
`
}

func (generator *generator) generateSource(
	classes []generatedClass,
	methods []generatedMethod,
) string {
	var output strings.Builder
	output.WriteString("#include \"area512_ti_builtin_db.h\"\n\n")

	writeBytePool(&output, "ti_builtin_name_pool", generator.namePool.data.Bytes())
	writeBytePool(&output, "ti_builtin_signature_pool", generator.signaturePool.data.Bytes())
	writeBytePool(&output, "ti_builtin_document_pool", generator.documentPool.data.Bytes())

	output.WriteString("const TiBuiltinClass ti_builtin_classes[] = {\n")
	for _, classEntry := range classes {
		fmt.Fprintf(
			&output,
			"  {%d, %d, %d, %d, %d},\n",
			classEntry.nameOffset,
			classEntry.instanceMethodStart,
			classEntry.instanceMethodCount,
			classEntry.classMethodStart,
			classEntry.classMethodCount,
		)
	}
	output.WriteString("};\n\n")

	output.WriteString("const TiBuiltinMethod ti_builtin_methods[] = {\n")
	for _, methodEntry := range methods {
		fmt.Fprintf(
			&output,
			"  {%d, %d, %d, %d, %d, %d, %d, %d, %d},\n",
			methodEntry.nameOffset,
			methodEntry.signatureOffset,
			methodEntry.documentOffset,
			methodEntry.returnClassID,
			methodEntry.returnArrayVariantClassID,
			methodEntry.returnUnionIndex,
			methodEntry.arrayVariantUnionIndex,
			methodEntry.blockParamClassID,
			methodEntry.originClassID,
		)
	}
	output.WriteString("};\n\n")

	output.WriteString("const TiBuiltinUnion ti_builtin_unions[] = {\n")
	for _, unionEntry := range generator.unions.entries {
		fmt.Fprintf(
			&output,
			"  {{%d, %d, %d, %d}},\n",
			unionEntry[0],
			unionEntry[1],
			unionEntry[2],
			unionEntry[3],
		)
	}
	output.WriteString("};\n\n")

	fmt.Fprintf(&output, "const uint16_t ti_builtin_class_count = %d;\n", len(classes))
	fmt.Fprintf(&output, "const uint16_t ti_builtin_method_count = %d;\n", len(methods))
	fmt.Fprintf(&output, "const uint16_t ti_builtin_union_count = %d;\n", len(generator.unions.entries))
	fmt.Fprintf(&output, "const uint16_t ti_builtin_name_pool_size = %d;\n", generator.namePool.data.Len())
	fmt.Fprintf(&output, "const uint16_t ti_builtin_signature_pool_size = %d;\n", generator.signaturePool.data.Len())
	fmt.Fprintf(&output, "const uint16_t ti_builtin_document_pool_size = %d;\n", generator.documentPool.data.Len())

	return output.String()
}

func writeBytePool(output *strings.Builder, name string, data []byte) {
	fmt.Fprintf(output, "const char %s[] = {\n", name)

	for offset := 0; offset < len(data); offset += 16 {
		end := offset + 16
		if end > len(data) {
			end = len(data)
		}

		output.WriteString("  ")
		for index, value := range data[offset:end] {
			if index > 0 {
				output.WriteByte(' ')
			}
			fmt.Fprintf(output, "0x%02x,", value)
		}
		output.WriteByte('\n')
	}

	output.WriteString("};\n\n")
}
