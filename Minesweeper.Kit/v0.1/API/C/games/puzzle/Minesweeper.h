/* Minesweeper Kit C API - Minesweeper.h
   __  __
  /  \/  \  __ ___  ____   ______ __ ______ ____ ____  ____ ____
 /	  \(__)   \/  -_)_/  _/  /  / /  -_)  -_)  _ \/  -_)  _/
/___/__/__/__/__/_/\___/____/ |______/\___/\___/  ___/\___/__/
(C) 2012-2018 Manuel Sainz de Baranda y Goñi. /__/
Released under the terms of the GNU Lesser General Public License v3. */

#ifndef __games_puzzle_Minesweeper_H__
#define __games_puzzle_Minesweeper_H__

#include <Z/types/base.h>
#include <Z/keys/status.h>

#ifndef MINESWEEPER_API
#	ifdef MINESWEEPER_STATIC
#		define MINESWEEPER_API
#	else
#		define MINESWEEPER_API Z_API
#	endif
#endif

#define MINESWEEPER_MINIMUM_X_SIZE     4
#define MINESWEEPER_MINIMUM_Y_SIZE     4
#define MINESWEEPER_MINIMUM_MINE_COUNT 2

#define MINESWEEPER_CELL_MASK_EXPLODED	(1 << 7)
#define MINESWEEPER_CELL_MASK_MINE	(1 << 6)
#define MINESWEEPER_CELL_MASK_DISCLOSED (1 << 5)
#define MINESWEEPER_CELL_MASK_FLAG	(1 << 4)
#define MINESWEEPER_CELL_MASK_WARNING	0xF

typedef zuint8 MinesweeperCell;

#define MINESWEEPER_CELL_EXPLODED( cell) ((cell) & MINESWEEPER_CELL_MASK_EXPLODED )
#define MINESWEEPER_CELL_MINE(	   cell) ((cell) & MINESWEEPER_CELL_MASK_MINE	  )
#define MINESWEEPER_CELL_DISCLOSED(cell) ((cell) & MINESWEEPER_CELL_MASK_DISCLOSED)
#define MINESWEEPER_CELL_FLAG(	   cell) ((cell) & MINESWEEPER_CELL_MASK_FLAG	  )
#define MINESWEEPER_CELL_WARNING(  cell) ((cell) & MINESWEEPER_CELL_MASK_WARNING  )

typedef zuint8 MinesweeperState;

#define MINESWEEPER_STATE_INITIALIZED 0
#define MINESWEEPER_STATE_PRISTINE    1
#define MINESWEEPER_STATE_PLAYING     2
#define MINESWEEPER_STATE_EXPLODED    3
#define MINESWEEPER_STATE_SOLVED      4

typedef zuint8 MinesweeperResult;

#define MINESWEEPER_RESULT_ALREADY_DISCLOSED 1
#define MINESWEEPER_RESULT_IS_FLAG	     2
#define MINESWEEPER_RESULT_MINE_FOUND	     3
#define MINESWEEPER_RESULT_SOLVED	     4

typedef struct Minesweeper Minesweeper;

#ifdef MINESWEEPER_USE_CALLBACK
	typedef void (* MinesweeperCellUpdated)(void*		   context,
						Minesweeper const* minesweeper,
						Z2DUInt		   cell_coordinates,
						MinesweeperCell	   cell_value);
#endif

struct Minesweeper {
	MinesweeperCell* matrix;
	Z2DUInt		 size;
	zuint		 mine_count;
	zuint		 remaining_count;
	zuint		 flag_count;
	MinesweeperState state;

#	ifdef MINESWEEPER_USE_CALLBACK
		MinesweeperCellUpdated cell_updated;
		void*		       cell_updated_context;
#	endif
};

Z_DEFINE_STRICT_STRUCTURE(
	zuint64		 x;
	zuint64		 y;
	zuint64		 mine_count;
	MinesweeperState state;
) MinesweeperSnapshotHeader;

Z_C_SYMBOLS_BEGIN

MINESWEEPER_API void		  minesweeper_initialize	(Minesweeper*	    object);

MINESWEEPER_API void		  minesweeper_finalize		(Minesweeper*	    object);

MINESWEEPER_API ZStatus		  minesweeper_prepare		(Minesweeper*	    object,
								 Z2DUInt	    size,
								 zuint		    mine_count);

MINESWEEPER_API zuint		  minesweeper_covered_count	(Minesweeper const* object);

MINESWEEPER_API zuint		  minesweeper_disclosed_count	(Minesweeper const* object);

MINESWEEPER_API MinesweeperResult minesweeper_disclose		(Minesweeper*	    object,
								 Z2DUInt	    coordinates);

MINESWEEPER_API MinesweeperResult minesweeper_toggle_flag	(Minesweeper*	    object,
								 Z2DUInt	    coordinates,
								 zboolean*	    new_state);

MINESWEEPER_API void		  minesweeper_disclose_all_mines(Minesweeper*	    object);

MINESWEEPER_API void		  minesweeper_flag_all_mines	(Minesweeper*	    object);

MINESWEEPER_API zboolean	  minesweeper_hint		(Minesweeper*	    object,
								 Z2DUInt*	    coordinates);

MINESWEEPER_API void		  minesweeper_resolve		(Minesweeper*	    object);

MINESWEEPER_API zusize		  minesweeper_snapshot_size	(Minesweeper const* object);

MINESWEEPER_API void		  minesweeper_snapshot		(Minesweeper const* object,
								 void*		    output);

MINESWEEPER_API ZStatus		  minesweeper_set_snapshot	(Minesweeper*	    object,
								 void*		    snapshot,
								 zusize		    snapshot_size);

#ifdef MINESWEEPER_USE_CALLBACK
	MINESWEEPER_API void minesweeper_set_cell_updated_callback(Minesweeper* object,
								   void*	cell_updated,
								   void*	cell_updated_context);
#endif

MINESWEEPER_API ZStatus minesweeper_snapshot_test  (void const*	      snapshot,
						    zusize	      snapshot_size);

MINESWEEPER_API void	minesweeper_snapshot_values(void const*	      snapshot,
						    zusize*	      snapshot_size,
						    Z2DUInt*	      size,
						    zuint*	      mine_count,
						    MinesweeperState* state);

Z_C_SYMBOLS_END

#endif /* __games_puzzle_Minesweeper_H__ */
