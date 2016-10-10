import code
import os
import sys
import subprocess
import time
import tempfile
import platform
sys.path.append(os.path.abspath('tools/pyscripts'))

import atexit
import colorama
from colorama import Fore, Back, Style

# --------------------------------------------------------------------------------------------------------------------
# Launched when scons is finished
def PrintInformationOnBuildIsFinished(startTimeInSeconds):
	timeDelta = time.gmtime(time.time() - startTimeInSeconds)
	print Style.BRIGHT + Fore.GREEN + time.strftime("Build time: %M minutes %S seconds", timeDelta) + Style.RESET_ALL

# --------------------------------------------------------------------------------------------------------------------

def main():
	archive = False
	if sys.argv[-1] == '--archive':
		archive = True
	timeStart = time.time()
	atexit.register(PrintInformationOnBuildIsFinished, timeStart)

	print platform.system()

	if sys.platform == 'darwin':
		if not os.path.isdir(r'./build'):
			os.mkdir('./build')
		os.chdir('build')
		buildCmd = ['cmake', '-G', 'Xcode', '../']
		execute(buildCmd)
		
	elif platform.system() == 'Windows':
		if not os.path.isdir(r'./build64'):
			os.mkdir('./build64')
		os.chdir('build64')
		print 'building Windows64'
		buildCmd = ['cmake', '-G', "Visual Studio 14 2015 Win64", '../']
		execute(buildCmd)
		
# --------------------------------------------------------------------------------------------------------------------

def execute(cmd, quiet = False, cwd = '.'):
	returnedCode = 0
	consoleOut = None
	consoleError = None
	if quiet:
		consoleOut = subprocess.PIPE
		consoleError = subprocess.STDOUT
	process = subprocess.Popen(cmd, stdout = consoleOut, stderr = consoleError, cwd = cwd)
	process.communicate()
	if process.wait() != 0:
		print >> sys.stderr, Style.BRIGHT + Fore.RED + 'Error occurred while executing following command:' + Style.RESET_ALL
		for argument in cmd:
			sys.stderr.write(argument+" ")
		sys.stderr.flush()
		sys.exit()


# --------------------------------------------------------------------------------------------------------------------

if __name__ == "__main__":
	main()
