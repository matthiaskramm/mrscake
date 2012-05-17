all: mrscake.html

../mrscake.so: ../mrscake.py.c
	cd ..;$(MAKE)
mrscake.html: ../mrscake.so
	PYTHONPATH=.. pydoc -w mrscake
