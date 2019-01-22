import fnmatch
import os
import time
import atexit
from SCons.Defaults import *

release = True
GUI = True

if GUI:
    GUIdef = ['-DEMUL86_GUI']
else:
    GUIdef = []

if(release):
	optimization = ['-O2', '-DNDEBUG'] + GUIdef
	debug = '-g0'
	lto = "1"
	closure = "0"
	assertions = "0"
	demangle = "0"
else:
	optimization = ['-O0'] + GUIdef
	debug = '-g3'
	lto = "0"
	closure = "0"
	assertions = "2"
	demangle = "1"


def main():
	env = Environment(ENV = os.environ, tools = ['gcc', 'g++', 'gnulink', 'ar', 'gas'])
		
	env.Replace(CC     = "emcc"    )
	env.Replace(CXX    = "em++"    )
	env.Replace(LINK   = "emcc"    )
	
	env.Replace(AR     = "emar"    )
	env.Replace(RANLIB = "emranlib")
	
	env.Replace(LIBLINKPREFIX = "")
	env.Replace(LIBPREFIX = "")
	env.Replace(LIBLINKSUFFIX = ".bc")
	env.Replace(LIBSUFFIX = ".bc")
 	env.Replace(OBJSUFFIX  = ".o")
 	env.Replace(PROGSUFFIX = ".html")
	
	env.Append( CPPFLAGS=optimization)
	env.Append( LINKFLAGS=[
		optimization,
		debug,
		"-lGL",
		"-lglfw",
		"-lGLEW",
		'-s', 'USE_GLFW=3',
		"-s", "ASSERTIONS=" + assertions,
		"-s", "DEMANGLE_SUPPORT=" + demangle,
		"-s", "TOTAL_MEMORY=52428800" + (" * 2" if GUI else ""),
        "-s", "EXTRA_EXPORTED_RUNTIME_METHODS=[\"ccall\", \"cwrap\"]",
		"--llvm-lto", lto,
		"--closure", closure,
		"-s", "NO_EXIT_RUNTIME=1",
		"-s", "DISABLE_EXCEPTION_CATCHING=1",
		"-s", "EXPORTED_FUNCTIONS=\"['_main','_emStart','_emPause','_emReboot']\"",
		"--preload-file", "c.img",
		"--preload-file", "imgui.ini"] + ([
		"--preload-file", "Dos3.3.img",
		"--preload-file", "Dos5.0.img",
		"--preload-file", "freedos722.img",
		"--preload-file", "Dos4.01.img",
		"--preload-file", "mikeos.dmg",
		"--preload-file", "Dos6.22.img"] if GUI else [])
	)

	timeStart = time.time()
	atexit.register(PrintInformationOnBuildIsFinished, timeStart)
	
	Includes = [
		"libs/glm",
		"libs/imgui",
		"libs/SimpleText/include",
	]
	
	imguiSources = [
		"libs/imgui/imgui.cpp",
		"libs/imgui/imgui_demo.cpp",
		"libs/imgui/imgui_widgets.cpp",
		"libs/imgui/imgui_draw.cpp",
	]
	
	sourcesPath = "sources"
	files = GlobR(sourcesPath, "*.cpp")
	
	env.Library('imgui', imguiSources, CPPPATH = Includes)
	
	program = env.Program('emul86', files, LIBS=['imgui'], CPPFLAGS=optimization + ['-std=c++14',  debug], LIBPATH='.', CPPPATH = Includes)
	
	env.Depends(program, GlobR(".", "c.img"))
	env.Depends(program, GlobR(".", "imgui.ini"))
	
def PrintInformationOnBuildIsFinished(startTimeInSeconds):
	""" Launched when scons is finished """
	failures = GetBuildFailures()
	for failure in failures:
		print "Target [%s] failed: %s" % (failure.node, failure.errstr)
	timeDelta = time.gmtime(time.time() - startTimeInSeconds)
	print time.strftime("Build time: %M minutes %S seconds", timeDelta)
	
def GlobR(path, filter) : 
	matches = []
	for root, dirnames, filenames in os.walk(path):
  		for filename in fnmatch.filter(filenames, filter):
   			matches.append(os.path.join(root, filename)) 
	return matches

if __name__ == "SCons.Script":
	main()
