/*
  Wilson Medeiros
  https://github.com/Mortanius
*/
typedef enum {
  NONE, BELOW_MIN, ABOVE_MAX
} measure_error;

// Water Tank parameters
// useful reference: https://www.fortlev.com.br/wp-content/uploads/2020/01/Manual_tecnico_fortlev_caixa_dagua_2020-06.pdf
// We will approximate the water tank to a truncated cone
typedef struct {
  // capacity in liters
  uint16_t capacity;
  // capacity error, in liters
  float capacityError;
  // dimensions in centimeters
  uint16_t height;
  uint16_t diameterMin;
  uint16_t diameterMax;
} WaterTank;
