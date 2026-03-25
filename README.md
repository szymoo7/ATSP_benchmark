# ATSP (C++, STL)

Projekt implementuje obiektowe rozwiazanie Asymetrycznego Problemu Komiwojazera (ATSP) bez bibliotek zewnetrznych.

## Zawartosc

- `TSPData` - model danych (N + macierz odleglosci)
- `TSPLIBParser` - parser danych TSPLIB (sekcja `EDGE_WEIGHT_SECTION`)
- `Timer` - pomiar czasu w mikrosekundach
- `Result` - DTO wyniku algorytmu
- `IAlgorithm` - interfejs algorytmow
- `BruteForceAlgorithm` - pelny przeglad bez rekurencji (`std::next_permutation`)
- `RandomSearchAlgorithm` - losowe permutacje z limitem czasu [ms]
- `NearestNeighborAlgorithm` - zachlanny NN
- `RepetitiveNNAlgorithm` - NN uruchamiany dla kazdego wierzcholka startowego
- `Menu` - konsolowe UI w petli
- `Benchmark` - automatyczny inteligentny benchmark z mapowaniem do najblizszych plikow TSPLIB i zapisem CSV

## Przycinanie rozmiaru N

Przed uruchomieniem algorytmow (`Brute Force`, `RAND`, `NN`, `RNN`) aplikacja pyta o rozmiar testu:

`Podaj rozmiar N do przetestowania (lub wpisz 0, aby uzyc pelnej macierzy):`

- `0` lub `N == pelny rozmiar` - uruchomienie na calej macierzy,
- `2 <= N < pelny rozmiar` - uruchomienie na przycietej macierzy `N x N` (lewy gorny wycinek),
- bledne `N` - komunikat i ponowna prosba o wartosc.

## Budowanie i uruchomienie (Windows / PowerShell)

```powershell
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug
.\cmake-build-debug\PEA_project1.exe
```

## Przykladowy plik

W repozytorium znajduje sie `sample.atsp` do szybkich testow.

Przykladowa sekwencja w menu:
1. Opcja `1` -> podaj sciezke: `sample.atsp`
2. Opcja `2` -> podglad macierzy
3. Opcje `3..6` -> uruchomienie algorytmow i wybor `N`
4. Opcja `7` -> automatyczny benchmark i zapis do `benchmark_results.csv`

