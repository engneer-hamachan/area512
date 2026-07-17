package main

import (
	"bytes"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func testConfigDirectory(t *testing.T) string {
	t.Helper()

	suggestions := []string{
		filepath.Join("..", "..", "template", "app", ".ti-config"),
		filepath.Join("template", "app", ".ti-config"),
	}

	for _, suggestion := range suggestions {
		if _, err := os.Stat(suggestion); err == nil {
			return suggestion
		}
	}

	t.Fatal("could not locate .ti-config")
	return ""
}

func testGenerator(t *testing.T) *generator {
	t.Helper()

	classes, err := loadClasses(testConfigDirectory(t))
	if err != nil {
		t.Fatal(err)
	}

	classIDs := make(map[string]uint8)
	for classID, className := range classNames {
		if className != "" {
			classIDs[className] = uint8(classID)
		}
	}

	return &generator{
		classes:       classes,
		classIDs:      classIDs,
		namePool:      newStringPool(),
		signaturePool: newStringPool(),
		documentPool:  newStringPool(),
		unions:        newUnionPool(),
	}
}

func findGeneratedMethod(
	methods []flatMethod,
	name string,
) (flatMethod, bool) {
	for _, method := range methods {
		if method.method.Name == name {
			return method, true
		}
	}

	return flatMethod{}, false
}

func TestFlattenMethods(t *testing.T) {
	generator := testGenerator(t)
	methods := generator.flattenMethods("Array", false)

	eachWithIndex, found := findGeneratedMethod(methods, "each_with_index")
	if !found {
		t.Fatal("Array is missing Enumerable#each_with_index")
	}
	if eachWithIndex.origin != "Enumerable" {
		t.Fatalf("unexpected origin: %s", eachWithIndex.origin)
	}

	if _, found := findGeneratedMethod(methods, "to_s"); !found {
		t.Fatal("Array is missing Object#to_s")
	}
}

func TestSyntheticTrueClass(t *testing.T) {
	generator := testGenerator(t)
	methods := generator.flattenMethods("TrueClass", false)

	if _, found := findGeneratedMethod(methods, "!"); !found {
		t.Fatal("TrueClass is missing Bool methods")
	}
}

func TestResolveTypes(t *testing.T) {
	generator := testGenerator(t)

	self := generator.resolveTypes([]string{"Self"}, "Array")
	if len(self.classIDs) != 1 || self.classIDs[0] != generator.classIDs["Array"] {
		t.Fatalf("unexpected Self resolution: %#v", self)
	}

	union := generator.resolveTypes([]string{"Int", "Float"}, "Integer")
	if len(union.classIDs) != 2 ||
		union.classIDs[0] != generator.classIDs["Integer"] ||
		union.classIDs[1] != generator.classIDs["Float"] {
		t.Fatalf("unexpected union resolution: %#v", union)
	}

	array := generator.resolveTypes([]string{"[String]"}, "Array")
	if len(array.arrayVariantClassIDs) != 1 ||
		array.arrayVariantClassIDs[0] != generator.classIDs["String"] {
		t.Fatalf("unexpected array resolution: %#v", array)
	}
}

func TestStringPoolOverflow(t *testing.T) {
	pool := newStringPool()
	pool.add(strings.Repeat("x", 65535))

	if !pool.overflowed {
		t.Fatal("string pool overflow was not detected")
	}
}

func TestGenerate(t *testing.T) {
	generator := testGenerator(t)
	classes, methods, err := generator.generate()
	if err != nil {
		t.Fatal(err)
	}

	if len(classes) != len(classNames) {
		t.Fatalf("got %d classes", len(classes))
	}
	if len(methods) == 0 {
		t.Fatal("method table is empty")
	}
	if generator.namePool.data.Len() > 65535 ||
		generator.signaturePool.data.Len() > 65535 ||
		generator.documentPool.data.Len() > 65535 {
		t.Fatal("a string pool exceeds 64KB")
	}

	stringClass := classes[generator.classIDs["String"]]
	start := int(stringClass.instanceMethodStart)
	end := start + int(stringClass.instanceMethodCount)
	foundSplit := false

	for _, method := range methods[start:end] {
		name := generator.namePool.data.Bytes()[method.nameOffset:]
		if len(name) >= 6 && string(name[:6]) == "split\x00" {
			foundSplit = true
			break
		}
	}

	if !foundSplit {
		t.Fatal("String#split was not generated")
	}
}

func TestStringSubSignature(t *testing.T) {
	generator := testGenerator(t)
	methods := generator.flattenMethods("String", false)
	sub, found := findGeneratedMethod(methods, "sub")

	if !found {
		t.Fatal("String#sub was not generated")
	}

	signature := generator.signature(sub.method, "String")
	if signature != "sub(String, String) -> String" {
		t.Fatalf("unexpected signature: %s", signature)
	}
}

func TestSignaturePrefixes(t *testing.T) {
	generator := testGenerator(t)
	methods := generator.flattenMethods("Sprite", true)
	constructor, found := findGeneratedMethod(methods, "new")

	if !found {
		t.Fatal("Sprite.new was not generated")
	}

	signature := generator.signature(constructor.method, "Sprite")
	if signature != "new(Integer, Integer, Integer?) -> Sprite" {
		t.Fatalf("unexpected signature: %s", signature)
	}

	if readSplatPrefix([]string{"**String"}) != "**" {
		t.Fatal("double splat prefix was not preserved")
	}
}

func TestGeneratedFilesAreCurrent(t *testing.T) {
	generator := testGenerator(t)
	classes, methods, err := generator.generate()
	if err != nil {
		t.Fatal(err)
	}

	generatedDirectory := filepath.Join(
		"..",
		"..",
		"components",
		"area512",
		"mrbgems",
		"picoruby-area512-ti",
		"src",
		"generated",
	)

	expectedFiles := map[string][]byte{
		"area512_ti_builtin_db.h": []byte(generateHeader()),
		"area512_ti_builtin_db.c": []byte(generator.generateSource(classes, methods)),
	}

	for fileName, expected := range expectedFiles {
		actual, err := os.ReadFile(filepath.Join(generatedDirectory, fileName))
		if err != nil {
			t.Fatal(err)
		}
		if !bytes.Equal(actual, expected) {
			t.Fatalf("%s is stale", fileName)
		}
	}
}
