#ifndef GATT_DATABASE_H_
#define GATT_DATABASE_H_

#include "bg_types.h"
#include "mible_api.h"
#include "gatt_db.h"

typedef struct{
	uint8_t number_of_characteristics;
	mible_gatts_char_db_t *p_characteristics;
} gatt_database_t;

#endif
