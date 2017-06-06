#!/usr/bin/Rscript

library(data.table)
library(plyr)
library(scales)
library(ggplot2)
require(tikzDevice)

options(echo = TRUE)
args <- commandArgs(trailingOnly = TRUE)

# set program
# prog <- args[1]
prog <- "loop-static"

baseFile <- paste0("data/", prog, "-base.csv")
optFile <- paste0("data/", prog, "-opt.csv")
bestFile <- paste0("data/", prog, "-best.csv")

baseData <- read.csv(baseFile)
optData <- read.csv(optFile)
bestData <- read.csv(bestFile)

# set machine column
baseData$Machine = "base"
optData$Machine = "my"
bestData$Machine = "LLVM"

# combine all data
total <- rbind(baseData, optData, bestData)

# calculate stuff
total.summary <- ddply(total, .(Machine, count), summarise
                       ,time.mean = mean(time)
                       ## ,time.sd = sd(time),
                       )

# set ouput file
tikz(paste0(prog, "-time.tex"))

# plot it

## mean time
## pdf(paste0(prog, "-time.pdf"))
tikz(paste0(prog, "-time.tex"))
qplot(count, time.mean, data = total.summary,
      colour = Machine,
      xlim = c(0,max(total.summary$count)),
      ylim = c(0, max(total.summary$time)),
      xlab = "Unroll count",
      ylab = "Mean time"
      ) + geom_line()
dev.off()

## loc
## pdf(paste0(prog, "-loc.pdf"))
tikz(paste0(prog, "-loc.tex"))
qplot(count, loc, data = total,
      colour = Machine,
      xlim = c(0,max(total$count)),
      ylim = c(0, max(total$time)),
      xlab = "Unroll count",
      ylab = "LOC"
      ) + geom_line()
dev.off()
