# AI | Lab 4 | Supervised Learning Classifier
Python implementation of **KNN** and **Naive Bayes** classifiers.

## Requirements
- Python 3

## Usage    
### KNN
```shell
python3 learner.py -train /path/to/training_file.csv -test /path/to/test_file.csv -K 3 
```

### Naive Bayes
```shell
python3 learner.py -train test_cases/nb/case2/train.csv -test test_cases/nb/case2/test.csv -C 2
```

### Example
```shell
[sj2810@snappy1 ai_lab_4]$ python3 learner.py -train test_cases/nb/case2/train.csv -test test_cases/nb/case2/test.csv -C 2 -verbose
P(C=1) = [20 / 84]
P(A0=1 | C=1) = 17 / 24
P(A1=1 | C=1) = 17 / 24
P(A2=1 | C=1) = 12 / 24
NB(C=1) = 0.059730
P(C=2) = [34 / 84]
P(A0=1 | C=2) = 10 / 38
P(A1=1 | C=2) = 22 / 38
P(A2=1 | C=2) = 18 / 38
NB(C=2) = 0.029211
P(C=3) = [30 / 84]
P(A0=1 | C=3) = 17 / 34
P(A1=1 | C=3) = 22 / 34
P(A2=1 | C=3) = 12 / 34
NB(C=3) = 0.040781
match: "1"
P(C=1) = [20 / 84]
P(A0=1 | C=1) = 17 / 24
P(A1=1 | C=1) = 17 / 24
P(A2=2 | C=1) = 12 / 24
NB(C=1) = 0.059730
P(C=2) = [34 / 84]
P(A0=1 | C=2) = 10 / 38
P(A1=1 | C=2) = 22 / 38
P(A2=2 | C=2) = 20 / 38
NB(C=2) = 0.032456
P(C=3) = [30 / 84]
P(A0=1 | C=3) = 17 / 34
P(A1=1 | C=3) = 22 / 34
P(A2=2 | C=3) = 22 / 34
NB(C=3) = 0.074765
fail: got "3" != want "2"
P(C=1) = [20 / 84]
P(A0=1 | C=1) = 17 / 24
P(A1=2 | C=1) = 7 / 24
P(A2=1 | C=1) = 12 / 24
NB(C=1) = 0.024595
P(C=2) = [34 / 84]
P(A0=1 | C=2) = 10 / 38
P(A1=2 | C=2) = 16 / 38
P(A2=1 | C=2) = 18 / 38
NB(C=2) = 0.021244
P(C=3) = [30 / 84]
P(A0=1 | C=3) = 17 / 34
P(A1=2 | C=3) = 12 / 34
P(A2=1 | C=3) = 12 / 34
NB(C=3) = 0.022244
fail: got "1" != want "3"
P(C=1) = [20 / 84]
P(A0=1 | C=1) = 17 / 24
P(A1=2 | C=1) = 7 / 24
P(A2=2 | C=1) = 12 / 24
NB(C=1) = 0.024595
P(C=2) = [34 / 84]
P(A0=1 | C=2) = 10 / 38
P(A1=2 | C=2) = 16 / 38
P(A2=2 | C=2) = 20 / 38
NB(C=2) = 0.023605
P(C=3) = [30 / 84]
P(A0=1 | C=3) = 17 / 34
P(A1=2 | C=3) = 12 / 34
P(A2=2 | C=3) = 22 / 34
NB(C=3) = 0.040781
fail: got "3" != want "1"
P(C=1) = [20 / 84]
P(A0=2 | C=1) = 7 / 24
P(A1=1 | C=1) = 17 / 24
P(A2=1 | C=1) = 12 / 24
NB(C=1) = 0.024595
P(C=2) = [34 / 84]
P(A0=2 | C=2) = 28 / 38
P(A1=1 | C=2) = 22 / 38
P(A2=1 | C=2) = 18 / 38
NB(C=2) = 0.081790
P(C=3) = [30 / 84]
P(A0=2 | C=3) = 17 / 34
P(A1=1 | C=3) = 22 / 34
P(A2=1 | C=3) = 12 / 34
NB(C=3) = 0.040781
match: "2"
P(C=1) = [20 / 84]
P(A0=2 | C=1) = 7 / 24
P(A1=1 | C=1) = 17 / 24
P(A2=2 | C=1) = 12 / 24
NB(C=1) = 0.024595
P(C=2) = [34 / 84]
P(A0=2 | C=2) = 28 / 38
P(A1=1 | C=2) = 22 / 38
P(A2=2 | C=2) = 20 / 38
NB(C=2) = 0.090878
P(C=3) = [30 / 84]
P(A0=2 | C=3) = 17 / 34
P(A1=1 | C=3) = 22 / 34
P(A2=2 | C=3) = 22 / 34
NB(C=3) = 0.074765
fail: got "2" != want "3"
P(C=1) = [20 / 84]
P(A0=2 | C=1) = 7 / 24
P(A1=2 | C=1) = 7 / 24
P(A2=1 | C=1) = 12 / 24
NB(C=1) = 0.010127
P(C=2) = [34 / 84]
P(A0=2 | C=2) = 28 / 38
P(A1=2 | C=2) = 16 / 38
P(A2=1 | C=2) = 18 / 38
NB(C=2) = 0.059484
P(C=3) = [30 / 84]
P(A0=2 | C=3) = 17 / 34
P(A1=2 | C=3) = 12 / 34
P(A2=1 | C=3) = 12 / 34
NB(C=3) = 0.022244
fail: got "2" != want "1"
P(C=1) = [20 / 84]
P(A0=2 | C=1) = 7 / 24
P(A1=2 | C=1) = 7 / 24
P(A2=2 | C=1) = 12 / 24
NB(C=1) = 0.010127
P(C=2) = [34 / 84]
P(A0=2 | C=2) = 28 / 38
P(A1=2 | C=2) = 16 / 38
P(A2=2 | C=2) = 20 / 38
NB(C=2) = 0.066093
P(C=3) = [30 / 84]
P(A0=2 | C=3) = 17 / 34
P(A1=2 | C=3) = 12 / 34
P(A2=2 | C=3) = 22 / 34
NB(C=3) = 0.040781
match: "2"
Label=1 Precision=1/2 Recall=1/3
Label=2 Precision=2/4 Recall=2/3
Label=3 Precision=0/2 Recall=0/2
```

## Help and other options
You can access help on additional options as follows
```shell
[sj2810@snappy1 ai_lab_4]$ python3 learner.py -h
usage: learner.py [-h] -train TRAIN -test TEST [-K K] [-C C] [-verbose]

Generic Classification-Evaluation Program

optional arguments:
  -h, --help    show this help message and exit
  -train TRAIN  Training file path
  -test TEST    Test file path
  -K K          Number of neighbours for KNN classification
  -C C          Laplacian correction for Naive Bayes
  -verbose      Enable verbose output
```