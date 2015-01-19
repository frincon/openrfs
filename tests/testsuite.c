#include "../src/config.h"
#include <check.h>

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#include "../src/privs.c"



START_TEST(privs_can_change_privilegies)
{
	gid_t gid;
	uid_t uid;
	openrfs_drop_privs(65535, 65534);
	gid = getegid();
	uid = geteuid();
	ck_assert_int_eq(65534, gid);
	ck_assert_int_eq(65535, uid);

	openrfs_restore_privs();
	gid = getegid();
	uid = geteuid();
	ck_assert_int_eq(0, gid);
	ck_assert_int_eq(0, uid);
}
END_TEST

void
*drop_privs_and_sleep(void *uid_pointer)
{
	int *uid = (int *) uid_pointer;
	openrfs_drop_privs(*uid,0);
	int uid2 = geteuid();
	ck_assert_int_eq(uid2, *uid);
	for(;;) {
		sleep(1);
	}
}

void
*get_euid_thread()
{
	int *uid;
	uid = malloc(sizeof(int));
	*uid = geteuid();
	return (void *) uid;
}

START_TEST(privs_change_privs_only_on_thread)
{
	int uid = 65535;
	int *uid_res;
	pthread_t thread_changed;
	pthread_t thread_check;

	pthread_create(&thread_changed, NULL, drop_privs_and_sleep, &uid );
	sleep(2);
	pthread_create(&thread_check, NULL, get_euid_thread, NULL);
	pthread_join(thread_check,(void *) &uid_res);
	ck_assert_int_eq(0, *uid_res);
	pthread_cancel(thread_changed);
}
END_TEST



Suite * openrfs_suite(void)
{
    Suite *s;
    TCase *tc_privs;

    s = suite_create("openrfs");

    /* Core test case */
    tc_privs = tcase_create("privs");

    tcase_add_test(tc_privs, privs_can_change_privilegies);
    tcase_add_test(tc_privs, privs_change_privs_only_on_thread);
    suite_add_tcase(s, tc_privs);

    return s;
}


int main(void)
{
	    int number_failed;
	    Suite *s;
	    SRunner *sr;

	    s = openrfs_suite();
	    sr = srunner_create(s);

	    srunner_run_all(sr, CK_NORMAL);
	    number_failed = srunner_ntests_failed(sr);
	    srunner_free(sr);
	    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
