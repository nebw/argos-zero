import os
import re
import ast
import time
import datetime

class RatingManager(object):
  def __init__(self,  fileName="ratings", path = "./rating"):
    if not path[-1] == "/":
      path = path + "/"
    self.path = path 
    self.fileName = fileName
    self.ratings = self.readRatings()

  def readRatings(self):
    fileName, fileNum = self.getLastFile()
    if fileNum == -1:
      return {}
    else:
      return self.getRatingsFromFile(fileName)
     
  def getLastFile(self):
    lastNum = -1
    lastFileName = ""
    for file in os.listdir(self.path):
      if self.fileName in file:
        numString = re.sub("[^0-9]", "", file)
        if len(numString) == 0:
          continue
        num = int(numString)
        if num > lastNum:
          lastNum = num
          lastFileName = file
    return lastFileName, lastNum

  def getRatingsFromFile(self, fileName):
    ratings = {}
    foundModel = False
    playerName = ""
    with open(self.path + fileName, "r") as stats:
      for line in stats:
        if foundModel and line[0] == "[":
          ratings[playerName] = ast.literal_eval(line)
          foundModel = False
        elif line[0] == "[":
          playerName = line.replace('\n', '').replace('\r', '')
          foundModel = True
        else:
          foundModel = False
    return ratings

  def getRatingForModels(self, ArrayOfModelParamter = []):
    outputRatings = []
    for modelParam in ArrayOfModelParamter:
      modelParamStr = str(modelParam)
      if not self.ratings.has_key(modelParamStr):
        self.ratings[modelParamStr] = [1000, 40]
      outputRatings.append(self.ratings[modelParamStr])
    return outputRatings

  def getEloRatingOfModels(self, ArrayOfModelParamter=[]):
    rat = self.getRatingForModels(ArrayOfModelParamter)
    return [rat[0][0], rat[1][0]]

  def updateRatings(self, arrayOfModelParam = [], numWins = []):
    if len(arrayOfModelParam) != 2 or len(numWins) != 2:
      return
    self.createRatingIfNotDefined(arrayOfModelParam)

    firstParamStr = str(arrayOfModelParam[0])
    secondParamStr = str(arrayOfModelParam[1])
    for i in ([1] * numWins[0] + [0] * numWins[1]):
      # calculate new ELOs
      first = self.ratings[firstParamStr]
      second = self.ratings[secondParamStr]
      newFirstELO = self.calculateNewElo(first[0], second[0], i, first[1])
      newSecondELO = self.calculateNewElo(second[0], first[0], 1 - i, second[1])
      # update ELOs
      self.ratings[firstParamStr] = [newFirstELO, first[1]]
      self.ratings[secondParamStr] = [newSecondELO, second[1]]

    return self.getEloRatingOfModels(arrayOfModelParam)

  def createRatingIfNotDefined(self, modelParamArray):
    for modelParam in modelParamArray:
      modelParamStr = str(modelParam)
      if not self.ratings.has_key(modelParamStr):
        self.ratings[modelParamStr]=[1000, 40]

  def calculateNewElo(self, elo1, elo2, erg1, k=20):
      dif = elo2 - elo1
      e1 = 1/(1+pow(10, dif / 400.))
      return int(round((elo1+k*(erg1-e1)), 0))
      

  def writeRatings(self):
    _ , fileNum = self.getLastFile()
    with open(self.path + self.fileName + str(fileNum + 1), "w") as newRatingsFile:
      # write a time stamp:
      ts = time.time()
      st = datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')
      newRatingsFile.write("## "+ st+"\n")
      # write the ratings
      for modelParam in self.ratings:
        # print "writing"
        newRatingsFile.write(modelParam+"\n")
        newRatingsFile.write(str(self.ratings[modelParam])+"\n")
    return fileNum + 1
