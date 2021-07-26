BINARIES:=create_data decoder

SRC:= encode_huffman.py

all:	test

static_analysis:
	flake8 ${SRC}
	pylint ${SRC}
	mypy --strict ${SRC}

songs_in_huffman.h:	.data

.data:	create_data encode_huffman.py
	@echo "[-] From monkey.h to frequencies/delay.data files..."
	@./$<
	@head -705 frequencies_2.data > a ; mv a frequencies_2.data
	@echo 64831 >> frequencies_2.data
	@head -705 delay_2.data > a ; mv a delay_2.data
	@head -375 frequencies_3.data > a ; mv a frequencies_3.data
	@echo 65161 >> frequencies_3.data
	@head -375 delay_3.data > a ; mv a delay_3.data
	@echo "[-] From frequencies/delay.data files to songs_in_huffman.h..."
	./encode_huffman.py
	touch $@

create_data:	create_data.c
	${CC} -g -o $@ $<

decoder:	decoder.c .data songs_in_huffman.h
	${CC} -Wall -Wextra -g -o $@ $<

test:	${BINARIES} .data
	@echo "[-] Recompiling decoder with new songs_in_huffman.h..."
	@make decoder
	@echo "[-] Running decoder..."
	@./decoder
	@echo "[-] Comparing original frequencies/delay data files with decompressed ones..."
	@bash -c 'for i in 0 1 2 3 ; do diff delay_$${i}.data d$${i}.data ; done'
	@bash -c 'for i in 0 1 ; do diff frequencies_$${i}.data f$${i}.data ; done'
	@bash -c 'for i in 2 3 ; do bash -c "diff <(head -n -1 frequencies_$${i}.data) f$${i}.data" ; done'
	@echo "[-] All done."

clean:
	rm -rf .data songs_in_huffman.h *.data ${BINARIES} .mypy_cache/ __pycache__/
