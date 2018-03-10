import pexpect

class Player(object):
    def __init__(self, processPath, color):
      self.process = pexpect.spawn(' '.join(processPath))
      self.color = color
      self.enemyColor = "white" if color == "black" else "black"

    def runCommand(self, cmd):
        self.process.sendline(cmd)
        self.process.expect('= ')
        out = self.process.readline()
        out = out.decode('utf-8')
        out = out.strip()
        return out

    def getLineOfOutput(self):
        return self.process.stdout.readline()

    def checkReturnedValue(self, val):
      if "?" in val:
        raise ValueError(self.color + " has encountered an error!\noutput: " + val)

    def genAndPlayMove(self):
      out = self.runCommand("genmove " + self.color)
      return out
    
    # the move parameter HAS the = sign at the beginning
    def applyMoveOfOtherPlayer(self, move):
      return self.applyMove(self.enemyColor, move.replace('=', ''))

    def applyMove(self, color, move):
      out = self.runCommand("play " + color + " " + move)
      return out

    def terminate(self):
      self.process.terminate()
