//
// $Id$
//

//
// Copyright (c) 2011, Andrew Aksyonoff
// Copyright (c) 2011, Sphinx Technologies Inc
// All rights reserved
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License. You should have
// received a copy of the GPL license along with this program; if you
// did not, you can find it at http://www.gnu.org/
//

//
// Sphinx UDF helpers implementation
//

#include "sphinxudf.h"

#include <memory.h>
#include <stdlib.h>

#define SPH_UDF_MAX_FIELD_FACTORS	256
#define SPH_UDF_MAX_TERM_FACTORS	2048


/// helper function that must be called to initialize the SPH_UDF_FACTORS structure 
/// before it is passed to sphinx_factors_unpack
/// returns 0 on success
/// returns an error code on error
int sphinx_factors_init ( SPH_UDF_FACTORS * out )
{
	if ( !out )
		return 1;

	memset ( out, 0, sizeof(SPH_UDF_FACTORS) );
	return 0;
}

/// helper function that unpacks PACKEDFACTORS() blob into SPH_UDF_FACTORS structure
/// MUST be in sync with PackFactors() method in sphinxsearch.cpp
/// returns 0 on success
/// returns an error code on error
int sphinx_factors_unpack ( const unsigned int * in, SPH_UDF_FACTORS * out )
{
	const unsigned int * pack = in;
	SPH_UDF_FIELD_FACTORS * f;
	SPH_UDF_TERM_FACTORS * t;
	int i, size;

	if ( !in || !out )
		return 1;

	if ( out->field || out->term )
		return 1;

	// extract size, extract document-level factors
	size = *in++;
	out->doc_bm25 = *in++;
	out->doc_bm25a = *(float*)in++;
	out->matched_fields = *in++;
	out->doc_word_count = *in++;
	out->num_fields = *in++;

	// extract field-level factors
	if ( out->num_fields > SPH_UDF_MAX_FIELD_FACTORS )
		return 1;

	if ( out->num_fields > 0 )
		out->field = malloc ( out->num_fields*sizeof(SPH_UDF_FIELD_FACTORS) );

	for ( i=0; i<out->num_fields; i++ )
	{
		out->field[i].hit_count = *in++;
		if ( !out->field[i].hit_count )
			continue;

		f = &(out->field[i]);
		f->id = *in++;
		f->lcs = *in++;
		f->word_count = *in++;
		f->tf_idf = *(float*)in++;
		f->min_idf = *(float*)in++;
		f->max_idf = *(float*)in++;
		f->sum_idf = *(float*)in++;
		f->min_hit_pos = (int)*in++;
		f->min_best_span_pos = (int)*in++;
		f->exact_hit = *in++;
		f->max_window_hits = (int)*in++;
	}

	// extract term-level factors
	out->max_uniq_qpos = *in++;
	if ( out->max_uniq_qpos > SPH_UDF_MAX_TERM_FACTORS )
		return 1;

	if ( out->max_uniq_qpos > 0 )
		out->term = malloc ( out->max_uniq_qpos*sizeof(SPH_UDF_TERM_FACTORS) );

	for ( i=0; i<out->max_uniq_qpos; i++ )
	{
		t = &(out->term[i]);
		t->keyword_mask = *in++;
		if ( t->keyword_mask )
		{
			t->id = *in++;
			t->tf = (int)*in++;
			t->idf = *(float*)in++;
		}
	}

	// do a safety check, and return
	return ( size!=( (int)(in-pack) * (int)sizeof(unsigned int) ) ) ? 1 : 0;
}


/// helper function that must be called to free the memory allocated by the sphinx_factors_unpack
/// function call
/// returns 0 on success
/// returns an error code on error
int sphinx_factors_deinit ( SPH_UDF_FACTORS * out )
{
	if ( !out )
		return 1;

	free ( out->term );
	free ( out->field );

	return 0;
}

//
// $Id$
//