#include "checks.h"
#include <ctype.h>
#include <string.h>
#include "assertions.h"
#include "is_integerish.h"
#include "any_missing.h"
#include "all_missing.h"
#include "all_nchar.h"
#include "bounds.h"


/*********************************************************************************************************************/
/* Some helpers                                                                                                      */
/*********************************************************************************************************************/
static inline Rboolean isTRUE(SEXP x) {
    return (LOGICAL(x)[0] == TRUE);
}

static inline Rboolean isFALSE(SEXP x) {
    return (LOGICAL(x)[0] == FALSE);
}

static inline Rboolean is_scalar_na(SEXP x) {
    if (length(x) == 1) {
        switch(TYPEOF(x)) {
            case LGLSXP: return (LOGICAL(x)[0] == NA_LOGICAL);
            case INTSXP: return (INTEGER(x)[0] == NA_INTEGER);
            case REALSXP: return ISNAN(REAL(x)[0]);
            case STRSXP: return (STRING_ELT(x, 0) == NA_STRING);
        }
    }
    return FALSE;
}

static Rboolean check_strict_names(SEXP x) {
    const R_len_t nx = length(x);

    const char *str;
    for (R_len_t i = 0; i < nx; i++) {
        str = CHAR(STRING_ELT(x, i));
        while (*str == '.')
            str++;
        if (!isalpha(*str))
            return FALSE;
        for (; *str != '\0'; str++) {
            if (!isalnum(*str) && *str != '.' && *str != '_')
                return FALSE;
        }
    }
    return TRUE;
}

/*********************************************************************************************************************/
/* Shared check functions returning an intermediate msg_t                                                            */
/*********************************************************************************************************************/
static msg_t check_names(SEXP nn, SEXP type) {
    if (!isNull(type)) {
        const char *ctype = CHAR(STRING_ELT(type, 0));

        if (strcmp(ctype, "unnamed") == 0) {
            if (!isNull(nn))
                return Msg("Must be unnamed");
        } else if (strcmp(ctype, "named") == 0) {
            if (isNull(nn) || any_missing_string(nn) || !all_nchar(nn, 1))
                return Msg("Must be named");
        } else if (strcmp(ctype, "unique") == 0) {
            if (isNull(nn) || any_missing_string(nn) || any_duplicated(nn, FALSE) > 0 || !all_nchar(nn, 1))
                return Msg("Must be uniquely named");
        } else if (strcmp(ctype, "strict") == 0) {
            if (isNull(nn) || any_missing_string(nn) || any_duplicated(nn, FALSE) > 0 || !all_nchar(nn, 1))
                return Msg("Must be uniquely named");
            if (!check_strict_names(nn))
                return Msg("Must be named according to R's variable naming rules");
        } else {
            error("Unknown naming definition '%s'", ctype);
        }
    }

    return MSGT;
}

static msg_t check_vector_props(SEXP x, SEXP any_missing, SEXP all_missing, SEXP len, SEXP min_len, SEXP max_len, SEXP unique, SEXP names) {
    if (!isNull(len)) {
        assertCount(len, "len");
        R_len_t n = asInteger(len);
        if (length(x) != n)
            return Msgf("Must have length %i", n);
    }

    if (!isNull(min_len)) {
        assertCount(min_len, "min_len");
        R_len_t n = asInteger(min_len);
        if (length(x) < n)
            return Msgf("Must have length >= %i", n);
    }

    if (!isNull(max_len)) {
        assertCount(max_len, "max_len");
        R_len_t n = asInteger(max_len);
        if (length(x) > n)
            return Msgf("Must have length <= %i", n);
    }

    assertFlag(any_missing, "any.missing");
    if (isFALSE(any_missing) && any_missing_atomic(x))
        return Msg("Contains missing values");

    assertFlag(all_missing, "all.missing");
    if (isFALSE(all_missing) && all_missing_atomic(x))
        return Msg("Contains only missing values");

    assertFlag(unique, "unique");
    if (isTRUE(unique) && any_duplicated(x, FALSE) > 0)
        return Msg("Contains only missing values");

    return check_names(getAttrib(x, R_NamesSymbol), names);
}

static msg_t check_matrix_props(SEXP x, SEXP any_missing, SEXP min_rows, SEXP min_cols, SEXP rows, SEXP cols, SEXP row_names, SEXP col_names) {
    if (!isNull(min_rows) || !isNull(rows)) {
        R_len_t xrows = nrows(x);
        if (!isNull(min_rows)) {
            assertCount(min_rows, "min.rows");
            if (xrows < asInteger(min_rows))
                return Msgf("Must have at least %i rows", min_rows);
        }
        if (!isNull(rows)) {
            assertCount(rows, "rows");
            if (xrows != asInteger(rows))
                return Msgf("Must have at exactly %i rows", rows);
        }
    }
    if (!isNull(min_cols) || !isNull(cols)) {
        R_len_t xcols = ncols(x);
        if (!isNull(min_cols)) {
            assertCount(min_cols, "min.cols");
            if (xcols < asInteger(min_cols))
                return Msgf("Must have at least %i cols", min_cols);
        }
        if (!isNull(cols)) {
            assertCount(cols, "cols");
            if (xcols != asInteger(cols))
                return Msgf("Must have at exactly %i cols", cols);
        }
    }

    assertFlag(any_missing, "any.missing");
    if (isFALSE(any_missing) && any_missing_atomic(x))
        return Msg("Contains missing values");

    if (!isNull(row_names) || !isNull(col_names)) {
        msg_t msg;
        SEXP dn = getAttrib(x, R_DimNamesSymbol);

        msg = check_names(isNull(dn) ? R_NilValue : VECTOR_ELT(dn, 1), row_names);
        if (!msg.ok)
            return msg;
        msg = check_names(isNull(dn) ? R_NilValue : VECTOR_ELT(dn, 1), col_names);
        if (!msg.ok)
            return msg;
    }

    return MSGT;
}

/*********************************************************************************************************************/
/* Exported check functions                                                                                          */
/*********************************************************************************************************************/
SEXP c_check_character(SEXP x, SEXP min_chars, SEXP any_missing, SEXP all_missing, SEXP len, SEXP min_len, SEXP max_len, SEXP unique, SEXP names) {
    if (!isString(x) && !all_missing_atomic(x))
        return CRes("Must be a character");
    if (!isNull(min_chars)) {
        assertCount(min_chars, "min.chars");
        R_len_t n = asInteger(min_chars);
        if (n > 0 && !all_nchar(x, n))
            return CResf("All elements must have at least %i characters", n);
    }
    return mwrap(check_vector_props(x, any_missing, all_missing, len, min_len, max_len, unique, names));
}

SEXP c_check_complex(SEXP x, SEXP any_missing, SEXP all_missing, SEXP len, SEXP min_len, SEXP max_len, SEXP unique, SEXP names) {
    if (!isComplex(x) && !all_missing_atomic(x))
        return CRes("Must be complex");
    return mwrap(check_vector_props(x, any_missing, all_missing, len, min_len, max_len, unique, names));
}

SEXP c_check_dataframe(SEXP x, SEXP any_missing, SEXP min_rows, SEXP min_cols, SEXP rows, SEXP cols, SEXP row_names, SEXP col_names) {
    if (!isFrame(x))
        return CRes("Must be a data frame");
    return mwrap(check_matrix_props(x, any_missing, min_rows, min_cols, rows, cols, row_names, col_names));
}

SEXP c_check_factor(SEXP x, SEXP any_missing, SEXP all_missing, SEXP len, SEXP min_len, SEXP max_len, SEXP unique, SEXP names) {
    if (!isFactor(x) && !all_missing_atomic(x))
        return CRes("Must be a factor");
    return mwrap(check_vector_props(x, any_missing, all_missing, len, min_len, max_len, unique, names));
}

SEXP c_check_integer(SEXP x, SEXP lower, SEXP upper, SEXP any_missing, SEXP all_missing, SEXP len, SEXP min_len, SEXP max_len, SEXP unique, SEXP names) {
    if (!isInteger(x) && !all_missing_atomic(x))
        return CRes("Must be integer");
    msg_t msg = check_bounds(x, lower, upper);
    if (!msg.ok)
        return mwrap(msg);
    return mwrap(check_vector_props(x, any_missing, all_missing, len, min_len, max_len, unique, names));
}

SEXP c_check_integerish(SEXP x, SEXP tol, SEXP lower, SEXP upper, SEXP any_missing, SEXP all_missing, SEXP len, SEXP min_len, SEXP max_len, SEXP unique, SEXP names) {
    if (!isIntegerish(x, asReal(tol)) && !all_missing_atomic(x))
        return CRes("Must be integerish");

    msg_t msg = check_bounds(x, lower, upper);
    if (!msg.ok)
        return mwrap(msg);

    return mwrap(check_vector_props(x, any_missing, all_missing, len, min_len, max_len, unique, names));
}

SEXP c_check_list(SEXP x, SEXP any_missing, SEXP all_missing, SEXP len, SEXP min_len, SEXP max_len, SEXP unique, SEXP names) {
    if (!isNewList(x) || isFrame(x) || isNull(x))
        return CRes("Must be a list");
    return mwrap(check_vector_props(x, any_missing, all_missing, len, min_len, max_len, unique, names));
}

SEXP c_check_logical(SEXP x, SEXP any_missing, SEXP all_missing, SEXP len, SEXP min_len, SEXP max_len, SEXP unique, SEXP names) {
    if (!isLogical(x) && !all_missing_atomic(x))
        return CRes("Must be logical");
    return mwrap(check_vector_props(x, any_missing, all_missing, len, min_len, max_len, unique, names));
}

SEXP c_check_matrix(SEXP x, SEXP any_missing, SEXP min_rows, SEXP min_cols, SEXP rows, SEXP cols, SEXP row_names, SEXP col_names) {
    if (!isMatrix(x))
        return CRes("Must be a matrix");
    return mwrap(check_matrix_props(x, any_missing, min_rows, min_cols, rows, cols, row_names, col_names));
}

SEXP c_check_names(SEXP nn, SEXP type) {
    return mwrap(check_names(nn, type));
}

SEXP c_check_named(SEXP x, SEXP type) {
    if (length(x) == 0)
        return mwrap(MSGT);
    return mwrap(check_names(getAttrib(x, R_NamesSymbol), type));
}

SEXP c_check_numeric(SEXP x, SEXP lower, SEXP upper, SEXP any_missing, SEXP all_missing, SEXP len, SEXP min_len, SEXP max_len, SEXP unique, SEXP names) {
    if (!isNumeric(x) && !all_missing_atomic(x))
        return CRes("Must be numeric");
    msg_t msg = check_bounds(x, lower, upper);
    if (!msg.ok)
        return mwrap(msg);
    return mwrap(check_vector_props(x, any_missing, all_missing, len, min_len, max_len, unique, names));
}

SEXP c_check_vector(SEXP x, SEXP any_missing, SEXP all_missing, SEXP len, SEXP min_len, SEXP max_len, SEXP unique, SEXP names) {
    if (!isVector(x) || isFrame(x))
        return CRes("Must be a vector");
    return mwrap(check_vector_props(x, any_missing, all_missing, len, min_len, max_len, unique, names));
}

/*********************************************************************************************************************/
/* Check functions for scalars                                                                                       */
/*********************************************************************************************************************/
SEXP c_check_flag(SEXP x, SEXP na_ok) {
    Rboolean is_na = is_scalar_na(x);
    if (length(x) != 1 || (!isLogical(x) && !is_na))
        return CRes("Must be a logical flag");

    assertFlag(na_ok, "na.ok");
    if (is_na && !isTRUE(na_ok))
        return CRes("May not be NA");
    return ScalarLogical(TRUE);
}

SEXP c_check_count(SEXP x, SEXP na_ok, SEXP positive) {
    Rboolean is_na = is_scalar_na(x);
    if (length(x) != 1 || (!isIntegerish(x, INTEGERISH_DEFAULT_TOL) && !is_na))
        return CRes("Must be a count");
    if (is_na) {
        assertFlag(na_ok, "na.ok");
        if (!isTRUE(na_ok))
            return CRes("May not be NA");
    } else  {
        assertFlag(positive, "positive");
        const int xi = asInteger(x), pos = LOGICAL(positive)[0];
        if (xi < pos)
            return CResf("Must be >= %i", pos);
    }
    return ScalarLogical(TRUE);
}

SEXP c_check_number(SEXP x, SEXP na_ok, SEXP lower, SEXP upper) {
    Rboolean is_na = is_scalar_na(x);
    if (length(x) != 1 || (!isNumeric(x) && !is_na))
        return CRes("Must be a number");

    assertFlag(na_ok, "na.ok");
    if (is_na) {
        if (!isTRUE(na_ok))
            return CRes("May not be NA");
        return ScalarLogical(TRUE);
    }
    return mwrap(check_bounds(x, lower, upper));
}

SEXP c_check_string(SEXP x, SEXP na_ok) {
    Rboolean is_na = is_scalar_na(x);
    if (length(x) != 1 || (!isString(x) && !is_na))
        return CRes("Must be a string");

    assertFlag(na_ok, "na.ok");
    if (is_na && !isTRUE(na_ok))
        return CRes("May not be NA");
    return ScalarLogical(TRUE);
}