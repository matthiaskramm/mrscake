#!/usr/bin/python

import sys
import os
import time
import hashlib
import traceback
import stat

def run(cmd):
    fi = os.popen(cmd, "rb")
    output = fi.read()
    status = fi.close()
    if status is not None:
        raise IOError("command '%s' returned non zero value %d" % (cmd, status))
    return output

def exception_as_string():
    s = "Exception "+str(sys.exc_info()[0])
    info = sys.exc_info()[1]
    if info:
        s += " "+str(info)
    s += "\n"
    for l in traceback.extract_tb(sys.exc_info()[2]):
        s += "  File \"%s\", line %d, in %s\n" % (l[0],l[1],l[2])
        s += "    %s\n" % l[3]
    return s.strip()

# this function doesn't return
def restart_script():
    os.execv(sys.executable, [sys.executable] + sys.argv)

class Server:
    def __init__(self):
        pass

    def start(self):
        self.started  = 1
        try:
            self.pid = os.fork()
            if not self.pid:
                try:
                    os.execv("./mrscake-job-server", ["./mrscake-job-server"])
                finally:
                    os.abort()
        except:
            print "Error spawning server"
            self.pid = None

    def stop(self):
        if self.pid:
            os.kill(self.pid, 9)
            os.waitpid(self.pid, 0)
            self.pid = None
        os.system("killall -9 mrscake-job-server");
        self.started = 0

# distribute the load to the git server, by delaying
# instance startup by a random number of seconds
delay = ord(hashlib.md5(run("hostname")).digest()[0]) / 10.0
print "Waiting %.1f seconds before startup" % delay
time.sleep(delay)

server = Server()

while 1:
    try:
        current_revision = run("git rev-parse HEAD").strip()
        run("git fetch origin master 2>&1")
        server_revision = run("git rev-parse FETCH_HEAD").strip()

        updated = False
        if current_revision != server_revision:
            print "upgrading from revision %s to revision %s" % (current_revision, server_revision)
            script_mtime_before = os.stat("autoupdate.py")[stat.ST_MTIME]
            try:
                print run("git merge FETCH_HEAD")
            except:
                print "Error merging revision %s" % server_revision
            script_mtime_after = os.stat("autoupdate.py")[stat.ST_MTIME]
            if script_mtime_after != script_mtime_before:
                server.stop()
                restart_script()
            updated = True

        if updated or not server.started:
            server.stop()
            run("make mrscake-job-server")
            server.start()
    except:
        print exception_as_string()
        fi = open("/tmp/mrscake.err", "wb")
        fi.write(exception_as_string())
        fi.close()

    time.sleep(30)

