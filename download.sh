#!/bin/bash
curl -L -o sudoku.zip\
  https://www.kaggle.com/api/v1/datasets/download/bryanpark/sudoku
unzip *.zip
rm -rf *.zip