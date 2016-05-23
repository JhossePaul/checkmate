#include "guessType.h"
#include <string.h>

const char * guessType(SEXP x) {
    SEXP attr = getAttrib(x, R_ClassSymbol);
    if (!isNull(attr)) {
        const R_len_t n = length(attr);
        if (n == 1)
            return CHAR(STRING_ELT(attr, 0));

        /* Constuct name using [class1]/[class2]/... */
        static char buf[512];
        const char * tmp = CHAR(STRING_ELT(attr, 0));
        strncpy(buf, tmp, 512);
        R_len_t written = strlen(tmp);
        for (R_len_t i = 1; i < n; i++) {
            tmp = CHAR(STRING_ELT(attr, i));
            if (strlen(tmp) > 512 - written - 1)
                break;
            written += snprintf(buf + written, 512 - written, "/%s", tmp);
        }
        return buf;
    }

    attr = getAttrib(x, R_DimSymbol);
    if (!isNull(attr) && isVectorAtomic(x))
        return length(attr) == 2 ? "matrix" : "array";

    return type2char(TYPEOF(x));
}

SEXP c_guessType(SEXP x) {
    return ScalarString(mkChar(guessType(x)));
}
