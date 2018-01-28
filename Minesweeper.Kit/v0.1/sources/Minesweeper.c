/* Minesweeper Kit - Minesweeper.c
   __  __
  /  \/  \  __ ___  ____   ______ __ ______ ____ ____  ____ ____
 /	  \(__)   \/  -_)_/  _/  /  / /  -_)  -_)  _ \/  -_)  _/
/___/__/__/__/__/_/\___/____/ |______/\___/\___/  ___/\___/__/
(C) 2012-2018 Manuel Sainz de Baranda y Goñi. /__/
Released under the terms of the GNU Lesser General Public License v3. */

#include <Z/functions/base/Z2DValue.h>
#include <Z/macros/pointer.h>

#ifndef MINESWEEPER_STATIC
#	define MINESWEEPER_API Z_API_EXPORT
#endif

#ifdef MINESWEEPER_USE_LOCAL_HEADER
#	include "Minesweeper.h"
#else
#	include <games/puzzle/Minesweeper.h>
#endif

#ifdef MINESWEEPER_USE_C_STANDARD_LIBRARY
#	include <stdlib.h>
#	include <string.h>

#	define z_deallocate(block)			  free(block)
#	define z_reallocate(block, block_size)		  realloc(block, block_size)
#	define z_copy(block, block_size, output)	  memcpy(output, block, block_size)
#	define z_block_int8_set(block, block_size, value) memset(block, value, block_size)
#	define z_random					  random
#else
#	include <ZBase/allocation.h>
#	include <ZBase/block.h>
#	include <ZSystem/randomness.h>
#endif

#define RANDOM			    ((zuint)z_random())
#define EXPLODED		    MINESWEEPER_CELL_MASK_EXPLODED
#define DISCLOSED		    MINESWEEPER_CELL_MASK_DISCLOSED
#define MINE			    MINESWEEPER_CELL_MASK_MINE
#define FLAG			    MINESWEEPER_CELL_MASK_FLAG
#define WARNING			    MINESWEEPER_CELL_MASK_WARNING
#define HEADER(p)		    ((MinesweeperSnapshotHeader *)(p))
#define HEADER_SIZE		    ((zusize)sizeof(MinesweeperSnapshotHeader))
#define CELL(	    cell_x, cell_y) object->matrix[cell_y * object->size.x + cell_x]
#define CELL_LOCAL( cell_x, cell_y) matrix[(cell_y) * size.x + (cell_x)]
#define VALID(	    cell_x, cell_y) ((cell_x) < object->size.x && (cell_y) < object->size.y)
#define VALID_LOCAL(cell_x, cell_y) ((cell_x) < size.x && (cell_y) < size.y)
#define X(pointer)		    (((zuint)(pointer - object->matrix)) % object->size.x)
#define Y(pointer)		    (((zuint)(pointer - object->matrix)) / object->size.x)
#define MATRIX_END		    (object->matrix + object->size.x * object->size.y)

#ifndef MINESWEEPER_USE_CALLBACK
#	define	UPDATED(cell_point, cell) \
		object->cell_updated(object->cell_updated_context, object, cell_point, cell)
#endif

static Z2DSInt8 const offsets[] = {
	{-1, -1}, {0, -1}, {1, -1},
	{-1,  0},	   {1,	0},
	{-1,  1}, {0,  1}, {1,	1}
};


static void place_mines(Minesweeper *object, Z2DUInt but)
	{
	MinesweeperCell *cell, *near;
	Z2DSInt8 const *offset;
	zuint x, y, near_x, near_y, count = object->mine_count;

	while (count)
		{
		random_point:
		x = RANDOM % object->size.x;
		y = RANDOM % object->size.y;

		if (!(x == but.x && y == but.y) && !(*(cell = &CELL(x, y)) & MINE))
			{
			for (offset = offsets + 8; offset-- != offsets;)
				if (x == but.x + offset->x && y == but.y + offset->y)
					goto random_point;

			*cell |= MINE;

			for (offset = offsets + 8; offset-- != offsets;)
				if (VALID(near_x = x + offset->x, near_y = y + offset->y))
					{
					near = &CELL(near_x, near_y);
					*near = (*near & ~WARNING) | (*near &  WARNING) + 1;
					}

			count--;
			}
		}

	object->state = MINESWEEPER_STATE_PLAYING;
	}


static void disclose_cell(Minesweeper *object, Z2DUInt point)
	{
	MinesweeperCell *cell = &CELL(point.x, point.y);

	if (!(*cell & (DISCLOSED | FLAG)))
		{
		*cell |= DISCLOSED;
		object->remaining_count--;

#		ifdef MINESWEEPER_USE_CALLBACK
			if (object->cell_updated != NULL) UPDATED(point, *cell);
#		endif

		if (!(*cell & WARNING))
			{
			Z2DSInt8 const *offset = offsets + 8;
			Z2DUInt near;

			while (offset-- != offsets) if (
				VALID	(near.x = point.x + offset->x,
					 near.y = point.y + offset->y)
			)
				disclose_cell(object, near);
			}
		}
	}


static void count_hint_cases(Minesweeper const *object, zuint *counts)
	{
	MinesweeperCell const *cell = MATRIX_END;
	Z2DSInt8 const *offset;
	zuint x, y, near_x, near_y;

	counts[0] = 0;
	counts[1] = 0;
	counts[2] = 0;

	while (cell-- != object->matrix) if (!(*cell & (DISCLOSED | FLAG | MINE)))
		{
		counts[2]++;

		if (*cell & WARNING)
			{
			counts[1]++;
			y = Y(cell);
			x = X(cell);

			for (offset = offsets + 8; offset-- != offsets;) if (
				VALID(near_x = x + offset->x, near_y = y + offset->y) &&
				(CELL(near_x, near_y) & DISCLOSED)
			)
				{
				counts[0]++;
				break;
				}
			}
		}
	}


static Z2DUInt case0_hint(Minesweeper const *object, zuint index)
	{
	MinesweeperCell const *cell = MATRIX_END;
	Z2DSInt8 const *offset;
	zuint x, y, near_x, near_y;

	while (cell-- != object->matrix)
		if (!(*cell & (DISCLOSED | FLAG | MINE)) && (*cell & WARNING))
			{
			y = Y(cell);
			x = X(cell);

			for (offset = offsets + 8; offset-- != offsets;) if (
				VALID(near_x = x + offset->x, near_y = y + offset->y) &&
				(CELL(near_x, near_y) & DISCLOSED)
			)
				{
				if (!index--) return z_2d_type(UINT)(x, y);
				break;
				}
			}

	return z_2d_type_zero(UINT);
	}


static Z2DUInt case1_hint(Minesweeper const *object, zuint index)
	{
	MinesweeperCell const *cell = MATRIX_END;

	while (cell-- != object->matrix)
		if (!(*cell & (DISCLOSED | FLAG | MINE)) && (*cell & WARNING) && !index--)
			return z_2d_type(UINT)(X(cell), Y(cell));

	return z_2d_type_zero(UINT);
	}


static Z2DUInt case2_hint(Minesweeper const *object, zuint index)
	{
	MinesweeperCell const *cell = MATRIX_END;

	while (cell-- != object->matrix)
		if (!(*cell & (DISCLOSED | FLAG | MINE)) && !index--)
			return z_2d_type(UINT)(X(cell), Y(cell));

	return z_2d_type_zero(UINT);
	}


MINESWEEPER_API
void minesweeper_initialize(Minesweeper *object)
	{
	object->state	   = MINESWEEPER_STATE_INITIALIZED;
	object->size.x	   = 0;
	object->size.y	   = 0;
	object->mine_count = 0;
	object->matrix	   = NULL;

#	ifdef MINESWEEPER_USE_CALLBACK
		object->cell_updated	     = NULL;
		object->cell_updated_context = NULL;
#	endif
	}


MINESWEEPER_API
void minesweeper_finalize(Minesweeper *object)
	{z_deallocate(object->matrix);}


MINESWEEPER_API
ZStatus minesweeper_prepare(Minesweeper *object, Z2DUInt size, zuint mine_count)
	{
	zuint cell_count;

	if (size.x < MINESWEEPER_MINIMUM_X_SIZE || size.y < MINESWEEPER_MINIMUM_Y_SIZE)
		return Z_ERROR_TOO_SMALL;

	if (z_type_multiplication_overflows(UINT)(size.x, size.y))
		return Z_ERROR_TOO_BIG;

	if (	mine_count < MINESWEEPER_MINIMUM_MINE_COUNT ||
		mine_count > (cell_count = size.x * size.y) - 9
	)
		return Z_ERROR_INVALID_ARGUMENT;

	if (object->size.x * object->size.y != cell_count)
		{
		void *matrix = z_reallocate(object->matrix, cell_count);

		if (matrix == NULL) return Z_ERROR_NOT_ENOUGH_MEMORY;
		object->matrix = matrix;
		}

	z_block_int8_set(object->matrix, cell_count, 0);
	object->size		= size;
	object->state		= MINESWEEPER_STATE_PRISTINE;
	object->flag_count	= 0;
	object->mine_count	= mine_count;
	object->remaining_count = cell_count - mine_count;
	return Z_OK;
	}


MINESWEEPER_API
zuint minesweeper_covered_count(Minesweeper const *object)
	{return object->size.x * object->size.y - minesweeper_disclosed_count(object);}


MINESWEEPER_API
zuint minesweeper_disclosed_count(Minesweeper const *object)
	{
	return	((object->size.x * object->size.y) - object->mine_count) -
		object->remaining_count;
	}


MINESWEEPER_API
MinesweeperResult minesweeper_disclose(Minesweeper *object, Z2DUInt coordinates)
	{
	MinesweeperCell *cell = &CELL(coordinates.x, coordinates.y);

	if (object->state == MINESWEEPER_STATE_PRISTINE) place_mines(object, coordinates);
	if (*cell & DISCLOSED) return MINESWEEPER_RESULT_ALREADY_DISCLOSED;
	if (*cell & FLAG     ) return MINESWEEPER_RESULT_IS_FLAG;

	if (*cell & MINE)
		{
		*cell |= DISCLOSED | EXPLODED;
		object->state = MINESWEEPER_STATE_EXPLODED;
		return MINESWEEPER_RESULT_MINE_FOUND;
		}

	disclose_cell(object, coordinates);

	if (!object->remaining_count)
		{
		object->state = MINESWEEPER_STATE_SOLVED;
		return MINESWEEPER_RESULT_SOLVED;
		}

	return Z_OK;
	}


MINESWEEPER_API
MinesweeperResult minesweeper_toggle_flag(
	Minesweeper* object,
	Z2DUInt	     coordinates,
	zboolean*    new_value
)
	{
	MinesweeperCell *cell = &CELL(coordinates.x, coordinates.y);

	if (*cell & DISCLOSED) return MINESWEEPER_RESULT_ALREADY_DISCLOSED;

	if (*cell & FLAG)
		{
		object->flag_count--;
		*cell &= ~FLAG;
		}

	else	{
		object->flag_count++;
		*cell |= FLAG;
		}

#	ifdef MINESWEEPER_USE_CALLBACK
		if (object->cell_updated != NULL) UPDATED(coordinates, *cell);
#	endif

	if (new_value != NULL) *new_value = !!(*cell & FLAG);
	return Z_OK;
	}


MINESWEEPER_API
void minesweeper_disclose_all_mines(Minesweeper *object)
	{
	MinesweeperCell *cell = MATRIX_END;

#	ifdef MINESWEEPER_USE_CALLBACK
		if (object->cell_updated != NULL)
			{
			Z2DUInt point;

			for (point.x = object->size.x; point.x--;)
				for (point.y = object->size.y; point.y--;)
					if (*--cell & MINE)
						{
						*cell |= DISCLOSED;
						UPDATED(point, *cell);
						}
			}

		else
#	endif

	while (cell != object->matrix) if (*--cell & MINE) *cell |= DISCLOSED;
	}


MINESWEEPER_API
void minesweeper_flag_all_mines(Minesweeper *object)
	{
	MinesweeperCell *cell = MATRIX_END;

#	ifdef MINESWEEPER_USE_CALLBACK
		if (object->cell_updated != NULL)
			{
			Z2DUInt point;

			for (point.x = object->size.x; point.x--;)
				for (point.y = object->size.y; point.y--;)
					if (*--cell & MINE)
						{
						*cell |= FLAG;
						UPDATED(point, *cell);
						}
			}

		else
#	endif

	while (cell != object->matrix) if (*--cell & MINE) *cell |= FLAG;
	}


MINESWEEPER_API
zboolean minesweeper_hint(Minesweeper *object, Z2DUInt *coordinates)
	{
	zuint counts[3];

	if (	object->state == MINESWEEPER_STATE_EXPLODED ||
		object->state == MINESWEEPER_STATE_SOLVED   ||
		object->state == MINESWEEPER_STATE_INITIALIZED
	)
		return FALSE;

	if (object->state == MINESWEEPER_STATE_PRISTINE)
		{
		place_mines(object, *coordinates = z_2d_type
			(UINT)(RANDOM % object->size.x, RANDOM % object->size.y));

		return TRUE;
		}

	count_hint_cases(object, counts);
	if	(counts[0]) *coordinates = case0_hint(object, RANDOM % counts[0]);
	else if (counts[1]) *coordinates = case1_hint(object, RANDOM % counts[1]);
	else if (counts[2]) *coordinates = case2_hint(object, RANDOM % counts[2]);
	else return FALSE;
	return TRUE;
	}


MINESWEEPER_API
void minesweeper_resolve(Minesweeper *object)
	{
	MinesweeperCell *cell = MATRIX_END;

#	ifdef MINESWEEPER_USE_CALLBACK
		if (object->cell_updated != NULL)
			{
			Z2DUInt point;

			for (point.x = object->size.x; point.x--;)
				for (point.y = object->size.y; point.y--;)
					if (!(*--cell & (MINE | DISCLOSED)))
						{
						*cell |= DISCLOSED;
						UPDATED(point, *cell);
						}
			}

		else
#	endif

	while (cell != object->matrix) if (!(*--cell & (MINE | DISCLOSED))) *cell |= DISCLOSED;
	object->remaining_count = 0;
	}


MINESWEEPER_API
zusize minesweeper_snapshot_size(Minesweeper const *object)
	{
	return object->state > MINESWEEPER_STATE_PRISTINE
		? HEADER_SIZE + object->size.x * object->size.y
		: HEADER_SIZE;
	}


MINESWEEPER_API
void minesweeper_snapshot(Minesweeper const *object, void *output)
	{
	HEADER(output)->x	   = z_uint64_big_endian(object->size.x);
	HEADER(output)->y	   = z_uint64_big_endian(object->size.y);
	HEADER(output)->mine_count = z_uint64_big_endian(object->mine_count);
	HEADER(output)->state	   = object->state;

	if (object->state > MINESWEEPER_STATE_PRISTINE)
		z_copy	(object->matrix, object->size.x * object->size.y,
			 (zuint8 *)output + HEADER_SIZE);
	}


MINESWEEPER_API
ZStatus minesweeper_set_snapshot(Minesweeper *object, void *snapshot, zusize snapshot_size)
	{
	MinesweeperCell *cell, *matrix;
	Z2DUInt size;
	zuint cell_count;
	ZStatus status = minesweeper_snapshot_test(snapshot, snapshot_size);

	if (status) return status;

	size = z_2d_type(UINT)
		((zuint)z_uint64_big_endian(HEADER(snapshot)->x),
		 (zuint)z_uint64_big_endian(HEADER(snapshot)->y));

	cell_count = size.x * size.y;

	if (cell_count != object->size.x * object->size.y)
		{
		if ((matrix = z_reallocate(object->matrix, cell_count)) == NULL)
			return Z_ERROR_NOT_ENOUGH_MEMORY;

		object->matrix = matrix;
		}

	else matrix = object->matrix;

	object->size		= size;
	object->mine_count	= (zuint)z_uint64_big_endian(HEADER(snapshot)->mine_count);
	object->state		= HEADER(snapshot)->state;
	object->flag_count	= 0;
	object->remaining_count = cell_count - object->mine_count;

	if (object->state <= MINESWEEPER_STATE_PRISTINE)
		z_block_int8_set(matrix, cell_count, 0);

	else	{
		z_copy((zuint8 *)snapshot + HEADER_SIZE, cell_count, matrix);

		for (cell = matrix + cell_count; cell-- != matrix;)
			{
			if (*cell & FLAG) object->flag_count++;
			if ((*cell & DISCLOSED) && !(*cell & MINE)) object->remaining_count--;
			}
		}

	return Z_OK;
	}


#ifdef MINESWEEPER_USE_CALLBACK

	MINESWEEPER_API
	void minesweeper_set_cell_updated_callback(
		Minesweeper* object,
		void*	     cell_updated,
		void*	     cell_updated_context
	)
		{
		object->cell_updated	     = cell_updated;
		object->cell_updated_context = cell_updated_context;
		}

#endif


MINESWEEPER_API
ZStatus minesweeper_snapshot_test(void const *snapshot, zusize snapshot_size)
	{
	Z2DUInt64	 size;
	zuint64		 mine_count;
	MinesweeperState state;
	zuint		 cell_count;

	if (	snapshot_size			   <  HEADER_SIZE		 ||
		((state = HEADER(snapshot)->state) == MINESWEEPER_STATE_PRISTINE &&
		 snapshot_size			   != HEADER_SIZE)
	)
		return Z_ERROR_INVALID_SIZE;

	if (	state == MINESWEEPER_STATE_INITIALIZED				 ||
		state >  MINESWEEPER_STATE_SOLVED				 ||
		(size.x = z_uint64_big_endian(HEADER(snapshot)->x))		 <
		MINESWEEPER_MINIMUM_X_SIZE					 ||
		(size.y = z_uint64_big_endian(HEADER(snapshot)->y))		 <
		MINESWEEPER_MINIMUM_Y_SIZE					 ||
		(mine_count = z_uint64_big_endian(HEADER(snapshot)->mine_count)) <
		MINESWEEPER_MINIMUM_MINE_COUNT
	)
		return Z_ERROR_INVALID_VALUE;

	if (
#		if Z_UINT_BITS < 64
			size.x > Z_UINT_MAXIMUM || size.y > Z_UINT_MAXIMUM ||
#		endif
		z_type_multiplication_overflows(UINT)((zuint)size.x, (zuint)size.y)
	)
		return Z_ERROR_TOO_BIG;

	if (mine_count > (cell_count = (zuint)size.x * (zuint)size.y) - 1)
		return Z_ERROR_INVALID_VALUE;

	if (state != MINESWEEPER_STATE_PRISTINE)
		{
		MinesweeperCell const *matrix = Z_BOP(void *, snapshot, HEADER_SIZE), *cell;
		Z2DSInt8 const *offset;
		zuint real_mine_count, exploded_count, x, y, near_x, near_y;
		zuint8 warning;

		if (snapshot_size != HEADER_SIZE + cell_count) return Z_ERROR_INVALID_SIZE;

		real_mine_count = 0;
		exploded_count	= 0;

		for (cell = matrix + cell_count; cell != matrix;)
			{
			cell--;

			/*-----------------------------------.
			| The flags can not be disclosed and |
			| only 1 exploded cell is allowed.   |
			'-----------------------------------*/
			if (	(*cell & FLAG && *cell & DISCLOSED) ||
				(*cell & EXPLODED && ++exploded_count > 1)
			)
				return Z_ERROR_INVALID_DATA;

			/*------------------------------------------.
			| The mines must be surrounded by warnings. |
			'------------------------------------------*/
			if (*cell & MINE)
				{
				real_mine_count++;

				x = (zuint)(cell - matrix) % size.x;
				y = (zuint)(cell - matrix) / size.x;

				for (offset = offsets + 8; offset-- != offsets;) if (
					VALID_LOCAL
						(near_x = x + offset->x,
						 near_y = y + offset->y)
					&& !(CELL_LOCAL(near_x, near_y) & WARNING)
				)
					return Z_ERROR_INVALID_DATA;
				}

			/*---------------------------------------.
			| The warning numbers must be surrounded |
			| by the correct amount of mines.	 |
			'---------------------------------------*/
			if (*cell & WARNING)
				{
				x = (zuint)(cell - matrix) % size.x;
				y = (zuint)(cell - matrix) / size.x;
				warning = 0;

				for (offset = offsets + 8; offset-- != offsets;) if (
					VALID_LOCAL
						(near_x = x + offset->x,
						 near_y = y + offset->y)
					&& (CELL_LOCAL(near_x, near_y) & MINE)
				)
					warning++;

				if (warning != (*cell & WARNING)) return Z_ERROR_INVALID_DATA;
				}
			}

		if (mine_count != real_mine_count) return Z_ERROR_INVALID_DATA;
		}

	return Z_OK;
	}


MINESWEEPER_API
void minesweeper_snapshot_values(
	void const*	  snapshot,
	zusize*		  snapshot_size,
	Z2DUInt*	  size,
	zuint*		  mine_count,
	MinesweeperState* state
)
	{
	zuint64	size_x = z_uint64_big_endian(HEADER(snapshot)->x);
	zuint64 size_y = z_uint64_big_endian(HEADER(snapshot)->y);

	if (snapshot_size != NULL) *snapshot_size = HEADER_SIZE + (zusize)(size_x * size_y);

	if (size != NULL)
		{
		size->x = (zuint)size_x;
		size->y = (zuint)size_y;
		}

	if (mine_count != NULL)
		*mine_count = (zuint)z_uint64_big_endian(HEADER(snapshot)->mine_count);

	if (state != NULL) *state = HEADER(snapshot)->state;
	}


/* Minesweeper.c EOF */
