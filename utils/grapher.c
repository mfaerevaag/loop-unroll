#!/usr/bin/Rscript

library(data.table)
library(plyr)
library(scales)
library(ggplot2)
library(ggthemr)
## require(tikzDevice)

## ggplot2 theme
## https://github.com/cttobin/ggthemr
ggthemr('flat dark')

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
baseData$Opt = "Base"
optData$Opt = "Simple"
bestData$Opt = "LLVM"

# combine all data
total <- rbind(baseData, optData, bestData)

# calculate stuff
total.summary <- ddply(total, .(Opt, count), summarise
                       ,time.mean = mean(time)
                       ## ,time.sd = sd(time),
                       )

# plot it

scaleX <- 10
scaleY <- 6

## mean time
pdf(paste0(prog, "-time.pdf"), width=scaleX, height=scaleY)
## tikz(paste0(prog, "-time.tex"))
qplot(count, time.mean, data = total.summary,
      colour = Opt, shape = Opt,
      xlim = c(0, max(total.summary$count)),
      ylim = c(0, max(total.summary$time)),
      xlab = "Unroll count",
      ylab = "Mean time"
      ) + geom_line()
dev.off()

## loc
pdf(paste0(prog, "-loc.pdf"), width=scaleX, height=scaleY)
## tikz(paste0(prog, "-loc.tex"))
qplot(count, loc, data = total,
      colour = Opt, shape = Opt,
      xlim = c(0, max(total$count)),
      ylim = c(100, max(total$loc)),
      xlab = "Unroll count",
      ylab = "LOC"
      ) + geom_line()
dev.off()
