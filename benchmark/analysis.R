library(dplyr)
library(jsonlite)
library(ggplot2)
library(tidyr)

setwd("~/Downloads")

# util
multiplot <- function(..., plotlist=NULL, file, cols=1, layout=NULL) {
  library(grid)

  # Make a list from the ... arguments and plotlist
  plots <- c(list(...), plotlist)

  numPlots = length(plots)

  # If layout is NULL, then use 'cols' to determine layout
  if (is.null(layout)) {
    # Make the panel
    # ncol: Number of columns of plots
    # nrow: Number of rows needed, calculated from # of cols
    layout <- matrix(seq(1, cols * ceiling(numPlots/cols)),
                     ncol = cols, nrow = ceiling(numPlots/cols))
  }

  if (numPlots==1) {
    print(plots[[1]])
  } else {
    # Set up the page
    grid.newpage()
    pushViewport(viewport(layout = grid.layout(nrow(layout), ncol(layout))))

    # Make each plot, in the correct location
    for (i in 1:numPlots) {
      # Get the i,j matrix positions of the regions that contain this subplot
      matchidx <- as.data.frame(which(layout == i, arr.ind = TRUE))

      print(plots[[i]], vp = viewport(layout.pos.row = matchidx$row,
                                      layout.pos.col = matchidx$col))
    }
  }
}

# load data
benchmarksgame_linux <- fromJSON("benchmarksgame-linux.json")
multiprocessing_linux <- fromJSON("multiprocessing-linux.json")
sequential_linux <- fromJSON("sequential-linux.json")
snakefish_linux <- fromJSON("snakefish-linux.json")
data_linux <- bind_rows(benchmarksgame_linux,
                        multiprocessing_linux,
                        sequential_linux,
                        snakefish_linux)

benchmarksgame_mac <- fromJSON("benchmarksgame-mac.json")
multiprocessing_mac <- fromJSON("multiprocessing-mac.json")
sequential_mac <- fromJSON("sequential-mac.json")
snakefish_mac <- fromJSON("snakefish-mac.json")
data_mac <- bind_rows(benchmarksgame_mac,
                        multiprocessing_mac,
                        sequential_mac,
                        snakefish_mac)

# process data
data_linux %>%
  select(c(times, bench_name)) %>%
  separate(bench_name, c("module", "program"), sep = '-') %>%
  mutate(module = factor(module)) %>%
  mutate(program = factor(program)) %>%
  unnest(times) ->
  run_times_linux

data_mac %>%
  select(c(times, bench_name)) %>%
  separate(bench_name, c("module", "program"), sep = '-') %>%
  mutate(module = factor(module)) %>%
  mutate(program = factor(program)) %>%
  unnest(times) ->
  run_times_mac

# run times box plots
run_times_linux %>%
  group_by(program) %>%
  mutate(times = times / min(times)) %>%
  ungroup() %>%
  filter(as.integer(program) != 2) %>% # filter dummy
  ggplot(aes(x = module, y = times, fill = module)) +
  geom_boxplot() +
  ylab("time (sec) / min time for this program across modules") +
  labs(title = "Linux") +
  coord_flip(ylim = c(1, 4.5)) ->
  boxplot_linux

run_times_mac %>%
  group_by(program) %>%
  mutate(times = times / min(times)) %>%
  ungroup() %>%
  filter(as.integer(program) != 2) %>% # filter dummy
  ggplot(aes(x = module, y = times, fill = module)) +
  geom_boxplot() +
  ylab("time (sec) / min time for this program across modules") +
  labs(title = "macOS") +
  coord_flip(ylim = c(1, 4.5)) ->
  boxplot_mac

# run times bar plots
run_times_linux %>%
  ggplot(aes(x = program, y = times)) +
  geom_col(aes(fill = module), position = "dodge2") +
  ylab("time (sec)") +
  labs(title = "Linux") +
  coord_flip(ylim = c(0, 8)) ->
  barplot_linux

run_times_mac %>%
  ggplot(aes(x = program, y = times)) +
  geom_col(aes(fill = module), position = "dodge2") +
  ylab("time (sec)") +
  labs(title = "macOS") +
  coord_flip(ylim = c(0, 8)) ->
  barplot_mac

run_times_linux %>%
  ggplot(aes(x = program, y = log10(times))) +
  geom_col(aes(fill = module), position = "dodge2") +
  ylab("log10 of time (sec)") +
  labs(title = "Linux") +
  coord_flip(ylim = c(-2, 1)) ->
  log_barplot_linux

run_times_mac %>%
  ggplot(aes(x = program, y = log10(times))) +
  geom_col(aes(fill = module), position = "dodge2") +
  ylab("log10 of time (sec)") +
  labs(title = "macOS") +
  coord_flip(ylim = c(-2, 1)) ->
  log_barplot_mac

# make plots
multiplot(boxplot_linux, boxplot_mac)
multiplot(barplot_linux, barplot_mac)
multiplot(log_barplot_linux, log_barplot_mac)

# ANOVA
run_times_linux %>%
  filter(as.integer(module) %in% c(2, 4)) ->
  tmp_linux

run_times_mac %>%
  filter(as.integer(module) %in% c(2, 4)) ->
  tmp_mac

summary(aov(times ~ module, data = tmp_linux))
summary(aov(times ~ module + program, data = tmp_linux))

summary(aov(times ~ module, data = tmp_mac))
summary(aov(times ~ module + program, data = tmp_mac))
