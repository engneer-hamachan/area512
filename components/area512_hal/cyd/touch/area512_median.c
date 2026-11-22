#include "area512_hal.h"

int
area512_sort_and_find_median(int *values, int value_count) {
  for (int i = 1; i < value_count; i++) {
    int value_to_insert = values[i];
    int j = i - 1;

    while (j >= 0 && values[j] > value_to_insert) {
      values[j + 1] = values[j];
      j--;
    }

    values[j + 1] = value_to_insert;
  }

  return values[value_count / 2];
}
