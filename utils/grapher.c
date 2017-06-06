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

# limit y-axis to shortest of each  machines max (or maybe not?)
## minMax <- min(c(
##     max(subset(total.summary, Machine == "CLR")$time.mean),
##     max(subset(total.summary, Machine == "JVM")$time.mean),
##     max(subset(total.summary, Machine == "Matisse")$time.mean)
##     ))

# set tex file
## tikz(paste0(prog, ".tex"))

# plot it
qplot(count, time.mean, data = total.summary,
      geom = "line", colour = Machine,
      xlim = c(0,max(total.summary$count)),
      ylim = c(0, max(total.summary$time)),
      #ylim = c(0, minMax),
      xlab = "Unroll count",
      ylab = "Mean time"
      )


## qplot(count, time.mean, data = total.summary,
##       geom = "line", colour = Machine,
##       xlim = c(0,max(total.summary$count)),
##       ylim = c(0, max(total.summary$time)),
##       ylab = "Mean-time"
##       #ylim = c(0, minMax),
##       ) + scale_x_log10(breaks = trans_breaks("log10", function(x) 10^x),
##                         labels = trans_format("log10", math_format(10^.x)))


# magic
## dev.off()
