context("checkFile")

td = tempfile()
dir.create(td, recursive=TRUE)
fn = file.path(td, "file")
dn = file.path(td, "dir")
ff = file.path(td, "xxx")
file.create(fn)
dir.create(dn)

test_that("checkFile", {
  expect_false(checkFile(ff))
  expect_false(checkFile(dn))
  expect_true(checkFile(fn))

  expect_error(assertFile(ff), "exist")
  expect_error(assertFile(dn), "directory")
  expect_true(assertFile(fn))
})

test_that("checkDirectory", {
  expect_false(checkDirectory(ff))
  expect_false(checkDirectory(fn))
  expect_true(checkDirectory(dn))

  expect_error(assertDirectory(ff), "exist")
  expect_error(assertDirectory(fn), "file")
  expect_true(assertDirectory(dn))
})

test_that("testAccess", {
  if (.Platform$OS.type != "windows" || TRUE) {
    Sys.chmod(fn, "0000")
    expect_true(makeCheckReturn(testAccess(fn, "")))
    expect_false(makeCheckReturn(testAccess(fn, "r")))
    expect_false(makeCheckReturn(testAccess(fn, "w")))
    expect_false(makeCheckReturn(testAccess(fn, "x")))
    Sys.chmod(fn, "0700")
    expect_true(makeCheckReturn(testAccess(fn, "")))
    expect_true(makeCheckReturn(testAccess(fn, "r")))
    expect_true(makeCheckReturn(testAccess(fn, "w")))
    expect_true(makeCheckReturn(testAccess(fn, "x")))
    Sys.chmod(fn, "0600")
    expect_true(makeCheckReturn(testAccess(fn, "")))
    expect_true(makeCheckReturn(testAccess(fn, "r")))
    expect_true(makeCheckReturn(testAccess(fn, "rw")))
    expect_false(makeCheckReturn(testAccess(fn, "rx")))
    expect_false(makeCheckReturn(testAccess(fn, "wx")))
  }
})