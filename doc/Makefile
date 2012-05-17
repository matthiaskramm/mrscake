all: mrscake.py.html

../mrscake.so: ../mrscake.py.c
	cd ..;$(MAKE)
mrscake.py.html: ../mrscake.so
	PYTHONPATH=.. pydoc -w mrscake && mv mrscake.html mrscake.py.html
