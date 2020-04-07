library(dplyr)
library(jsonlite)
library(ggplot2)
library(tidyr)

setwd("~/Downloads")

snakefish <- fromJSON("snakefish.json")
multiprocessing <- fromJSON("multiprocessing.json")
data <- bind_rows(snakefish, multiprocessing)

data %>%
  select(c(times, bench_name)) %>%
  separate(bench_name, c("module", "program"), sep = '-') %>%
  mutate(module = factor(module)) %>%
  mutate(program = factor(program)) %>%
  unnest(times) ->
  run_times

# run times boxplot
run_times %>%
  group_by(program) %>%
  mutate(times = times / min(times)) %>%
  ungroup() %>%
  filter(as.integer(program) != 2) %>% # filter dummy
  ggplot(aes(x = module, y = times)) +
  geom_boxplot() +
  ylab("time (sec) / min time for this program across modules") +
  coord_flip()

# run times bar plot
run_times %>%
  ggplot(aes(x = program, y = times)) +
  geom_col(aes(fill = module), position = "dodge2") +
  ylab("time (sec)") +
  coord_flip()

# ANOVA
summary(aov(times ~ module, data = run_times))
