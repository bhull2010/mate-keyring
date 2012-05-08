/* This is auto-generated code. Edit at your own peril. */
#include "testing/testing.h"
#include "test-suite.h"

typedef void (*TestingFunc)(int *, const void *);

static void start_tests (void) {
}

static void stop_tests (void) {
}

static void initialize_tests (void) {
	TestingFunc setup = NULL;
	TestingFunc teardown = NULL;
	setup = teardown = NULL;
g_test_add("/ssh_openssh/parse_public", int, NULL, setup, testing__test__parse_public, teardown);
g_test_add("/ssh_openssh/parse_private", int, NULL, setup, testing__test__parse_private, teardown);
	setup = teardown = NULL;
setup = testing__setup__private_key_setup;
teardown = testing__teardown__private_key_teardown;
g_test_add("/private_key/private_key_parse_plain", int, NULL, setup, testing__test__private_key_parse_plain, teardown);
g_test_add("/private_key/private_key_parse_and_unlock", int, NULL, setup, testing__test__private_key_parse_and_unlock, teardown);
	setup = teardown = NULL;
}

static void run_externals (int *ret) {
	testing_external_run ("testing__external__ssh_module", testing__external__ssh_module, ret);
}

static int run(void) {
	int ret;
	initialize_tests ();
	start_tests ();
	ret = g_test_run ();
	if (ret == 0)
		run_externals (&ret);
	stop_tests();
	return ret;
}
#include "testing/testing.c"
