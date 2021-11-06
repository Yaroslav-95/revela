#include "tests/tests.h"
#include "fs.h"

#include <time.h>
#include <string.h>

#define DATETIME_TEST_FILE "tests/empty"

static void
test_rbasename(void)
{
	char *a = "/path/to/hello.jpg";
	const char *ob;
	ob = rbasename(a);
	asserteq(strcmp(ob, "hello.jpg"), 0);
}

static void
test_joinpath(void)
{
	char *a = "hello", *b = "world.jpeg";
	char *joined = joinpath(a, b);
	asserteq(strcmp(joined, "hello/world.jpeg"), 0);
	free(joined);
}

static void
test_isimage(void)
{
	char *a = "hello.jpg", *b = "goodbye.jpeg", *c = "iamge.png", *d = "b.tiff";
	char *notimg = "image.exe";
	asserteq(isimage(a), true);
	asserteq(isimage(b), true);
	asserteq(isimage(c), true);
	asserteq(isimage(d), true);
	asserteq(isimage(notimg), false);
}

static void
test_delext(void)
{
	char *na = "hello.jpg", *nb = "goodbye.tar.gz";
	size_t bla = 16, blb = 16, blc = 4;
	char bufa[bla], bufb[blb], bufc[blc];
	delext(na, bufa, bla);
	asserteq(strcmp(bufa, "hello"), 0);
	delext(nb, bufb, blb);
	asserteq(strcmp(bufb, "goodbye.tar"), 0);
	delext(nb, bufc, blc);
}

static void
test_setdatetime_uptodate(void)
{
	time_t now = time(NULL);
	struct timespec mtim = {
		.tv_sec = now,
		.tv_nsec = 690,
	};
	asserteq(file_is_uptodate(DATETIME_TEST_FILE, &mtim), 0);
	setdatetime(DATETIME_TEST_FILE, &mtim);
	asserteq(file_is_uptodate(DATETIME_TEST_FILE, &mtim), 1);
}

int
main(void)
{
	INIT_TESTS();
	RUN_TEST(test_rbasename);
	RUN_TEST(test_joinpath);
	RUN_TEST(test_isimage);
	RUN_TEST(test_delext);
	RUN_TEST(test_setdatetime_uptodate);
}
