# ATSP Benchmark

## Opis projektu

Projekt realizuje różne algorytmy rozwiązywania problemu komiwojażera dla instancji ATSP (asymetryczny problem komiwojażera). Program umożliwia wczytywanie danych z plików, uruchamianie wybranych algorytmów oraz analizę wyników. Implementacja w C++ bez bibliotek zewnętrznych, z wykorzystaniem STL.

## Struktura projektu

- `main.cpp` – główny plik uruchamiający aplikację
- `src/` – implementacje algorytmów i logiki programu
- `include/` – pliki nagłówkowe
- `data/` – przykładowe pliki z danymi ATSP (format TSPLIB)


## Obsługiwane algorytmy

- Brute Force (przegląd zupełny)
- Nearest Neighbor
- Losowe przeszukiwanie (Random Search)
- Repetitive Nearest Neighbor

## Wykorzystane struktury danych i rozwiązania techniczne

- **Macierz odległości**: dynamicznie alokowana tablica wskaźników na tablice (`int**`) - każdy wiersz to osobna tablica długości N. Implementacja znajduje się w `include/TSPData.h`, z konstruktorami, operatorami kopiującymi/przenoszącymi oraz funkcją `getTruncatedData` tworzącą nową, przyciętą instancję.
- **Permutacje**: generowane przy użyciu `std::next_permutation` (Brute Force).
- **Wektory STL**: do przechowywania tras, permutacji, wyników i danych wejściowych (`std::vector<int>`, `std::vector<Result>`).
- **Mapy i słowniki**: do mapowania nazw plików i wyników benchmarków (`std::map`, `std::unordered_map`).
- **Interfejsy i polimorfizm**: interfejs `IAlgorithm` oraz klasy dziedziczące dla każdego algorytmu.
- **DTO**: klasa `Result` do przechowywania wyniku algorytmu (trasa, koszt, czas).
- **Parser TSPLIB**: własna implementacja wczytująca sekcję `EDGE_WEIGHT_SECTION` do macierzy odległości (`src/TSPLIBParser.cpp`).
- **Pomiar czasu**: klasa `Timer` oparta o `std::chrono` (mikrosekundy).
- **UI**: proste menu konsolowe w pętli (`Menu`).
- **Benchmark**: automatyczne uruchamianie algorytmów na wielu plikach i zapis wyników do CSV (`src/Benchmark.cpp`, `include/Benchmark.h`).

## Przycinanie rozmiaru N

Przed uruchomieniem algorytmów aplikacja pyta o rozmiar testu:

`Podaj rozmiar N do przetestowania (lub wpisz 0, aby użyć pełnej macierzy):`

- `0` lub `N == pełny rozmiar` – uruchomienie na całej macierzy,
- `2 <= N < pełny rozmiar` – uruchomienie na przyciętej macierzy `N x N` (lewy górny wycinek),
- błędne `N` – komunikat i ponowna prośba o wartość.

## Budowanie i uruchomienie (Windows / PowerShell)

```powershell
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --config Release
.\cmake-build-release\PEA_project1.exe
```

## Pliki wejściowe

Pliki z danymi ATSP znajdują się w katalogu `data/`. Obsługiwane są pliki w formacie TSPLIB (sekcja `EDGE_WEIGHT_SECTION`) i liczba wierzchołków, macierz.

Przykładowy fragment pliku ATSP (TSPLIB):

```
NAME : ftv33
TYPE : ATSP
DIMENSION : 33
EDGE_WEIGHT_TYPE : EXPLICIT
EDGE_WEIGHT_FORMAT : FULL_MATRIX
EDGE_WEIGHT_SECTION
0 1 2 3 ...
...
EOF
```

## Testowanie

Testowanie odbywa się manualnie poprzez uruchamianie programu i wybór opcji z menu. Wyniki benchmarków oraz działania algorytmów można weryfikować na podstawie plików wejściowych z katalogu `data/`. Wyniki automatycznych benchmarków zapisywane są do pliku `benchmark_results.csv`.

## Przykładowa sekwencja w menu

1. Opcja `1` → podaj ścieżkę: `sample.atsp` (lub wybierz plik z katalogu `data/`)
2. Opcja `2` → podgląd macierzy
3. Opcje `3..6` → uruchomienie algorytmów i wybór `N`
4. Opcja `7` → automatyczny benchmark i zapis do pliku `benchmark_results.csv` (w katalogu build)
5. Opcja `0` → zakończenie programu

## Kalibracja i dobór parametrów testów

1) Główne stałe i pliki konfiguracyjne (domyślne)

- Plik kalibracyjny: `ftv170.atsp` (`include/Benchmark.h` → `kCalibrationFileName`).
- Limit czasu pojedynczego pomiaru w kalibracji: `kTargetCalibrationTimeMicroseconds` = 10 minut (600 s).
- Twardy limit dla Brute Force: `kBruteForceHardMaxSize` = 15 (Brute Force nie będzie próbował N > 15).
- Liczba powtórzeń na punkt pomiarowy: `kTrialsPerMeasurement` = 5.
- Ustawienia testu zbieżności RAND: `kRandConvergenceTrials` = 5, oraz domyślne limity czasu RAND `kRandTimeLimitMs` = 10 min (używane przy domyślnych testach konwergencji).

Wszystkie wartości znajdują się w `include/Benchmark.h` i można je zmienić przed przebudowaniem programu.

2) Procedura wyznaczania `Nmax`

- Dla każdego algorytmu wywoływana jest funkcja `calibrateMaxSize`:
  - Dla Brute Force: startN = 5, krok = 1; maksymalna próbowana N to min(base_size, kBruteForceHardMaxSize).
  - Dla NN i RNN: startN = 10, krok początkowy = 10 (krok redukowany do 1 przy zbliżaniu się do limitu).
- Dla każdej kandydatowej wartości N tworzone jest `TSPData` przycięte z pliku kalibracyjnego (`getTruncatedData`) i uruchamiany jest pojedynczy przebieg algorytmu; jeśli zmierzony czas przekroczy `kTargetCalibrationTimeMicroseconds`, kalibracja zatrzymuje się.
- `Nmax` to największe N, dla którego mierzony średni czas (tutaj pojedyncze uruchomienie w kalibracji) jest mniejszy niż limit.

3) Strategia doboru punktów testowych i „pokrycie” między algorytmami

- Moduł tworzy dwie klasy punktów testowych:
  - Wspólne (małe) punkty referencyjne: `buildSharedSmallPoints` - generuje kilka wartości N blisko wykrytego `bfMaxSize` (np. bfMaxSize-2, bfMaxSize-1, bfMaxSize). Te punkty są wykonywane przez WSZYSTKIE algorytmy, by umożliwić bezpośrednie porównania.
  - Punkty rozszerzone/idealne: `buildExtendedIdealPoints` - interpoluje do 4 punktów między ostatnim wspólnym punktem a `algorithmMaxSize` (ograniczonym przez `baseSize` lub `Nmax`) i dla algorytmów heurystycznych mapuje je do najbliższych dostępnych plików TSPLIB (`mapToNearestDataset`).
- Dla Brute Force większość punktów pochodzi z przyciętego pliku kalibracyjnego (truncated base), natomiast heurystyki (NN, RNN, RAND) używają istniejących plików TSPLIB (z katalogu `data/`) dobranych metodą „nearest dataset”.
- Lista punktów zostaje posortowana i przycinana do maksymalnie 7 punktów na algorytm (implementacja: `buildPointsForAlgorithm`).

4) Parametry pomiarów i metryki zapisywane do CSV

- Dla każdego pomiaru (agregat z `kTrialsPerMeasurement` powtórzeń) zapisywane są następujące pola:
  - `Source` (np. nazwa pliku lub TRUNC(...))
  - `Algorithm` (BruteForce/RAND/NN/RNN)
  - `N` (użyty rozmiar)
  - `Avg_Time_us` (średni czas w mikrosekundach)
  - `Avg_Cost` (średni koszt)
  - `Best_Cost` (najlepszy z prób)
  - `Trial_Count` (liczba prób)
  - `Reference_Cost` (jeśli dostępny)
  - `Relative_Error_%` (wyliczony względem referencji, jeśli dostępna)
  - `Parameters` (dodatkowe informacje, np. Limit_... dla testu RAND)

- Nagłówek pliku zapisywanego przez program to (format programu):

  Source;Algorithm;N;Avg_Time_us;Avg_Cost;Best_Cost;Trial_Count;Reference_Cost;Relative_Error_%;Parameters

5) Test zbieżności RAND

- Moduł `runRandConvergenceTest` używa domyślnie instancji o docelowym rozmiarze `targetSize = 48` i listy limitów czasu (ms): `[1, 2, 5, 10, 50, 100, 500, 1000, 5000]`.
- Dla każdego limitu uruchamiane jest `kRandConvergenceTrials` niezależnych prób (z losowymi seedami), a wyniki zapisywane są jako dodatkowe wiersze w CSV z parametrem opisującym limit (np. `Limit_100ms`).

6) Gdzie zmieniać domyślne zachowanie benchmarku

- Stałe konfiguracyjne (czasy, liczba prób, nazwa pliku kalibracyjnego, twardy limit BF) znajdują się w `include/Benchmark.h`.
- Katalog i mapowanie dostępnych plików TSPLIB znajduje się w `src/Benchmark.cpp` w `kDatasetCatalog` - dopisz/usuwaj wpisy aby zmienić bazę instancji.
- Parametry specyficzne dla implementacji algorytmów (np. domyślny limit czasu RAND dla pojedynczego uruchomienia) znajdują się w `include/Benchmark.h` i/lub w konstruktorach odpowiednich klas algorytmów (`src/RandomSearchAlgorithm.cpp`).

## Autor

### Szymon Wesołowski

## Licencja

Projekt udostępniany na warunkach licencji MIT.
