AC_DEFUN([RFX_CHECK_PYTHON],
[
AC_MSG_CHECKING([for Python version])

if test "x$PYTHON_LIB" '!=' "x" -a "x$PYTHON_INCLUDES" '!=' "x";then
    # you can override the python detection by putting PYTHON_LIB
    # and PYTHON_INCLUDES into the environment
    case "$PYTHON_INCLUDES" in
        *python2.4*) PY_VERSION=2.4
	             ;;
        *python2.5*) PY_VERSION=2.5
	             ;;
        *python2.6*) PY_VERSION=2.6
	             ;;
        *python2.7*) PY_VERSION=2.7
	             ;;
        *python3.0*) PY_VERSION=3.0
	             ;;
        *python3.1*) PY_VERSION=3.1
	             ;;
        *python3.2*) PY_VERSION=3.2
	             ;;
        *python3.3*) PY_VERSION=3.3
	             ;;
        *python3.4*) PY_VERSION=3.4
	             ;;
        *)           PY_VERSION=unknown
	             ;;
    esac
else
    if test "x$PYTHON_LIB" '!=' "x" -o "x$PYTHON_INCLUDES" '!=' "x";then
        echo "Set both PYTHON_LIB and PYTHON_INCLUDES, or none at all"
    fi
    # iterate through version 2.4 to 3.4
    VERSIONS_2="2.4 2.5 2.6 2.7"
    VERSIONS_3="3.0 3.1 3.2 3.3 3.4"
    VERSIONS="$VERSIONS_2 $VERSIONS_3"
    case "$PYTHON" in
	2*) VERSIONS="$VERSIONS_2 $PYTHON"
	    ;;
	3*) VERSIONS="$VERSIONS_3 $PYTHON"
	    ;;
    esac
    for v in $VERSIONS; do
        # Linux
        if test -f "/usr/include/python$v/Python.h";then
            PY_VERSION=$v
	    PYTHON_LIB="-lpython$PY_VERSION"
	    PYTHON_INCLUDES="-I/usr/include/python$PY_VERSION"
        # Mac OS X
        elif test -f "/Library/Frameworks/Python.framework/Versions/$v/include/python$v/Python.h";then
            PY_VERSION=$v
            PYTHON_LIB="-framework Python" 
            PYTHON_INCLUDES="-I/Library/Frameworks/Python.framework/Versions/$v/include/python$v/"
	# Mac OS X [Fink]:
        elif test "(" -f "/sw/lib/python$v/config/libpython$v.dylib" \
	           -o -f "/sw/lib/python$v/config/libpython$v.a" \
	           -o -f "/sw/lib/python$v/config/libpython$v.so" \
	          ")" \
	       -a -f "/sw/include/python$v/Python.h"; then
            PY_VERSION=$v
            PYTHON_LIB="-L /sw/lib/python$v/config/ -lpython$PY_VERSION"
            PYTHON_INCLUDES="-I /sw/include/python$v/"
        fi
    done
fi
AC_MSG_RESULT($PY_VERSION)
    
if test "x$PY_VERSION" "!=" "x"; then
    AC_MSG_CHECKING([for Python executable])
    if python$PY_VERSION -V 2>&5;then
        PYTHON_EXECUTABLE=python$PY_VERSION
        AC_SUBST(PYTHON_EXECUTABLE)
        AC_MSG_RESULT([$PYTHON_EXECUTABLE])
    elif python -V >&5 2>&5;then
        if python -V 2>&1 | grep -q "\b$PY_VERSION"; then
          PYTHON_EXECUTABLE=python
          AC_SUBST(PYTHON_EXECUTABLE)
          AC_MSG_RESULT([$PYTHON_EXECUTABLE])
        else
          AC_MSG_RESULT(["'python' executable is not version $PY_VERSION"])
        fi
    else
        AC_MSG_RESULT([failed])
    fi
fi

if test "x$PY_VERSION" "!=" "x" -a "x$PYTHON_EXECUTABLE" "!=" "x"; then
    export PYTHON_INCLUDES PYTHON_LIB
    AC_SUBST(PYTHON_LIB)
    AC_SUBST(PYTHON_INCLUDES)
    AC_MSG_CHECKING([whether we can compile the Python test program])
    
    cat > conftest.c << EOF
#   include <Python.h>

    int main()
    {
	int ret;
	ret = Py_Main(0, 0);
	int x; // also check for gcc 2.95.x incompatibilities
	return ret;
    }
EOF
    
    ac_link='$CC $CPPFLAGS $CFLAGS $PYTHON_INCLUDES conftest.c $LDFLAGS $PYTHON_LIB $LIBS -o conftest${ac_exeext}'
    if { (eval echo python.m4: \"$ac_link\") 1>&5; (eval $ac_link) 2>&5; } && test -s conftest${ac_exeext}; then
	AC_MSG_RESULT(yes)
	PYTHON_OK=yes
    else
        echo "configure: failed program was:" >&5
        cat conftest.c >&5
        AC_MSG_RESULT(no)
    fi
    rm -f conftest*
    
    AC_MSG_CHECKING([for Python install path])
cat > _pypath.py << EOF
import distutils
import distutils.sysconfig
import sys
sys.stdout.write(distutils.sysconfig.get_python_lib(plat_specific=0,standard_lib=0))
EOF 
    echo $PYTHON_EXECUTABLE _pypath.py 1>&5
    if $PYTHON_EXECUTABLE _pypath.py >_pypath.txt 2>&5;then
    	PYTHON_INSTALL_PATH=`cat _pypath.txt`
        AC_SUBST(PYTHON_INSTALL_PATH)
        AC_MSG_RESULT($PYTHON_INSTALL_PATH)
    else
        AC_MSG_RESULT([failed])
    fi
    rm -f _pypath.txt _pypath.py
fi
])
