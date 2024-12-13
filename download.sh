#!/bin/bash
curl -L -o sudoku.zip\
  https://www.kaggle.com/api/v1/datasets/download/ninetentacles/sampled-sodoku-from-sodoku-3m
unzip *.zip
rm -rf *.zip

