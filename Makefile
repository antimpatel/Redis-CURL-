FILES:
	$(shell g++ -o SRC src.cpp lib/libhiredis.a -lcurl)
		
