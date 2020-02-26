#pragma once

namespace tarsx {
	struct Slice {
		explicit Slice(void* d = nullptr, size_t data_len = 0, size_t length = 0)
			:data(d), dataLen(data_len), len(length){
			
		}

		void* data;
		size_t dataLen;
		size_t len;
	};
}