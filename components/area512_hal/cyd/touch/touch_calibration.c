#include "touch_calibration.h"

#include <math.h>

#define TOUCH_CALIBRATION_CORNER_COUNT \
  (AREA512_TOUCH_CALIBRATION_VALUE_COUNT / 2)

static double
compute_determinant_3x3(const double matrix[3][3]) {
  return matrix[0][0] * (matrix[1][1] * matrix[2][2] -
                         matrix[1][2] * matrix[2][1]) -
         matrix[0][1] * (matrix[1][0] * matrix[2][2] -
                         matrix[1][2] * matrix[2][0]) +
         matrix[0][2] * (matrix[1][0] * matrix[2][1] -
                         matrix[1][1] * matrix[2][0]);
}

static bool
solve_3x3_linear_system(
  const double normal_matrix[3][3],
  const double normal_constants[3],
  double coefficients[3]
) {

  double determinant = compute_determinant_3x3(normal_matrix);

  if (fabs(determinant) < 1e-9)
    return false;

  for (int replaced_column_index = 0; replaced_column_index < 3;
       replaced_column_index++) {

    double replaced_matrix[3][3];

    for (int row_index = 0; row_index < 3; row_index++) {
      for (int i = 0; i < 3; i++)
        replaced_matrix[row_index][i] = i == replaced_column_index
                                          ? normal_constants[row_index]
                                          : normal_matrix[row_index][i];
    }

    coefficients[replaced_column_index] =
      compute_determinant_3x3(replaced_matrix) / determinant;
  }

  return true;
}

static bool
compute_raw_to_panel_coefficients(
  const TouchCalibrationPoint calibration_points[TOUCH_CALIBRATION_POINT_COUNT],
  bool is_computing_panel_y,
  double coefficients[3]
) {

  double normal_matrix[3][3] = {{0}};
  double normal_constants[3] = {0};

  for (int i = 0; i < TOUCH_CALIBRATION_POINT_COUNT; i++) {
    double raw_point_terms[3] = {
      calibration_points[i].raw_x,
      calibration_points[i].raw_y,
      1
    };
    double panel_coordinate = is_computing_panel_y
                                ? calibration_points[i].panel_y
                                : calibration_points[i].panel_x;

    for (int row_index = 0; row_index < 3; row_index++) {
      for (int column_index = 0; column_index < 3; column_index++)
        normal_matrix[row_index][column_index] +=
          raw_point_terms[row_index] * raw_point_terms[column_index];

      normal_constants[row_index] +=
        raw_point_terms[row_index] * panel_coordinate;
    }
  }

  return solve_3x3_linear_system(normal_matrix, normal_constants, coefficients);
}

bool
area512_compute_touch_calibration_values(
  const TouchCalibrationPoint calibration_points[TOUCH_CALIBRATION_POINT_COUNT],
  int panel_width,
  int panel_height,
  uint16_t calibration_values[AREA512_TOUCH_CALIBRATION_VALUE_COUNT]
) {

  double panel_x_coefficients[3];
  double panel_y_coefficients[3];

  if (
    !compute_raw_to_panel_coefficients(
      calibration_points,
      false,
      panel_x_coefficients
    ) ||
    !compute_raw_to_panel_coefficients(
      calibration_points,
      true,
      panel_y_coefficients
    )
  )
    return false;

  double linear_part_determinant =
    panel_x_coefficients[0] * panel_y_coefficients[1] -
    panel_x_coefficients[1] * panel_y_coefficients[0];

  if (fabs(linear_part_determinant) < 1e-12)
    return false;

  for (int corner_index = 0; corner_index < TOUCH_CALIBRATION_CORNER_COUNT;
       corner_index++) {

    // The corner order of Panel_Device::setCalibrate.
    double panel_corner_x = (corner_index & 2) ? panel_width - 1 : 0;
    double panel_corner_y = (corner_index & 1) ? panel_height - 1 : 0;
    double panel_corner_x_without_offset =
      panel_corner_x - panel_x_coefficients[2];
    double panel_corner_y_without_offset =
      panel_corner_y - panel_y_coefficients[2];
    double raw_corner_x =
      (panel_y_coefficients[1] * panel_corner_x_without_offset -
       panel_x_coefficients[1] * panel_corner_y_without_offset) /
      linear_part_determinant;
    double raw_corner_y =
      (-panel_y_coefficients[0] * panel_corner_x_without_offset +
       panel_x_coefficients[0] * panel_corner_y_without_offset) /
      linear_part_determinant;

    if (
      raw_corner_x < 0 || raw_corner_x > UINT16_MAX || raw_corner_y < 0 ||
      raw_corner_y > UINT16_MAX
    )
      return false;

    calibration_values[corner_index * 2] = (uint16_t)lround(raw_corner_x);
    calibration_values[corner_index * 2 + 1] = (uint16_t)lround(raw_corner_y);
  }

  return true;
}
