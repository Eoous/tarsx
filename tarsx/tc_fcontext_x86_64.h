#pragma once
#include <stdint.h>

namespace tarsx {
	extern "C"{
		struct stack_t {
			void* sp = 0;
			size_t size = 0;
		};

		struct fp_t {
			uint32_t fc_freg[2];
		};

		struct fcontext_t {
			uint64_t fc_greg[8];
			stack_t fc_stack;
			fp_t fc_fp;
		};
	}
}