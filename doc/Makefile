all: mrscake.py.html rdoc/index.html

../mrscake.so: ../mrscake.py.c
	cd ..;$(MAKE)
mrscake.py.html: ../mrscake.so
	PYTHONPATH=.. pydoc -w mrscake && mv mrscake.html mrscake.py.html
rdoc/index.html: ../mrscake.rb.c
	cd ..;rake -f doc/rdoc.Rakefile rdoc
