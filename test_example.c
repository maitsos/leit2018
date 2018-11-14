#include <stdio.h>
#include "unit_testing.h"
#include "kernel_sched.h"

BOOT_TEST(test_create_thread,
	"Test that a process thread can be created and joined. Also, that "
	"the argument of the thread is passed correctly."
	)
{
	int flag = 0;

	int task(int argl, void* args) {
		
		ASSERT(args == &flag);
		*(int*)args = 1;
		
		return 2;
	}

	Tid_t t = CreateThread(task, sizeof(flag), &flag);
	fprintf(stderr, "%s%lu\n", "RETURNED with ptcb tid:", t );
	ASSERT(t!=NOTHREAD);
	int exitval;
	ASSERT(ThreadJoin(t, &exitval)== 0);
	ASSERT(flag==1);

	return 0;
}


TEST_SUITE(all_my_tests, "These are mine")
{
  &test_create_thread,
  NULL
};

int main(int argc, char** argv)
{
  return register_test(&all_my_tests) ||
    run_program(argc, argv, &all_my_tests);
}

