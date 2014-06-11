#' Check if an object is an integerish vector
#'
#' @templateVar fn Integerish
#' @template na-handling
#' @template checker
#' @inheritParams checkInteger
#' @inheritParams checkVector
#' @param tol [\code{double(1)}]\cr
#'  Numerical tolerance used to check whether a double or complex can be converted.
#'  Default is \code{sqrt(.Machine$double.eps)}.
#' @family basetypes
#' @useDynLib checkmate c_is_integerish
#' @export
#' @examples
#'  testIntegerish(1L)
#'  testIntegerish(1.)
#'  testIntegerish(1:2, lower = 1L, upper = 2L, any.missing = FALSE)
checkIntegerish = function(x, tol = .Machine$double.eps^.5, lower = -Inf, upper = Inf, any.missing = TRUE, all.missing = TRUE, len = NULL, min.len = NULL, max.len = NULL, unique = FALSE, names = NULL) {
  .Call("c_check_integerish", x, tol, lower, upper, any.missing, all.missing, len, min.len, max.len, unique, names, PACKAGE = "checkmate")
}

#' @rdname checkIntegerish
#' @useDynLib checkmate c_is_integerish
#' @export
assertIntegerish = function(x, tol = .Machine$double.eps^.5, lower = -Inf, upper = Inf, any.missing = TRUE, all.missing = TRUE, len = NULL, min.len = NULL, max.len = NULL, unique = FALSE, names = NULL, .var.name) {
  makeAssertion(
    .Call("c_check_integerish", x, tol, lower, upper, any.missing, all.missing, len, min.len, max.len, unique, names, PACKAGE = "checkmate")
    , vname(x, .var.name))
}

#' @rdname checkIntegerish
#' @useDynLib checkmate c_is_integerish
#' @export
testIntegerish = function(x, tol = .Machine$double.eps^.5, lower = -Inf, upper = Inf, any.missing = TRUE, all.missing = TRUE, len = NULL, min.len = NULL, max.len = NULL, unique = FALSE, names = NULL) {
  isTRUE(
    .Call("c_check_integerish", x, tol, lower, upper, any.missing, all.missing, len, min.len, max.len, unique, names, PACKAGE = "checkmate")
  )
}
