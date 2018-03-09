from subprocess import Popen, PIPE

class Player(object):
    def __init__(self, processPath, color):
      self.process = Popen(processPath, stdin=PIPE, stdout=PIPE)
      self.color = color
      self.enemyColor = "white" if color == "black" else "black"

    def runCommand(self, cmd):
        self.process.stdin.write((cmd + '\n').encode('utf-8'))

    def getLineOfOutput(self):
        return self.process.stdout.readline()

    def getOutput(self):
      output = b""
      line = self.getLineOfOutput()
      print(line)
      while not line == "\n":
        output += line + b"\n"
        line = self.getLineOfOutput()
      
      self.checkReturnedValue(output)
      return output

    def checkReturnedValue(self, val):
      if "?" in val:
        raise ValueError(self.color + " has encountered an error!\noutput: " + val)

    def genAndPlayMove(self):
      self.runCommand("genmove " + self.color)
      return self.getOutput()
    
    # the move parameter HAS the = sign at the beginning
    def applyMoveOfOtherPlayer(self, move):
      return self.applyMove(self.enemyColor, move.replace('=', ''))

    def applyMove(self, color, move):
      self.runCommand("play " + color + " " + move)
      return self.getOutput()

    def terminate(self):
      self.process.terminate()
