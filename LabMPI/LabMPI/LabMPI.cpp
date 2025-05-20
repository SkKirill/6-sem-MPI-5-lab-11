#include<stdio.h>
#include<mpi.h>
#include<time.h>
#include <iostream>
#include <random>
#include <iomanip>
#include <limits>

using namespace std;

/*
Функция createArr() создает одномерный массив целых чисел заданного размера, заполняя его
случайными значениями от 0 до 20. Для генерации случайных чисел используются библиотеки random_device,
default_random_engine и uniform_int_distribution из стандартной библиотеки C++. Каждый элемент
массива заполняется случайным числом в диапазоне от min до max.
*/
void createArr(int* row, int size)
{
    const int min = 0, max = 20;
    random_device r;
    default_random_engine e(r());
    uniform_int_distribution<int> digit(min, max);
    for (int i = 0; i < size; i++)
    {
        row[i] = digit(e);
    }
}

void printArr(int* row, int size, string letter, int rank, string rowOrCal)
{
    cout << rowOrCal << " " << rank << " of matrix " << letter << ":";
    for (int i = 0; i < size; i++)
    {
        cout << setw(4) << row[i];
    }
    cout << endl;
}

int findMaxElem(int* row, int size) {
    int min = row[0];
    for (int i = 1; i < size; i++)
    {
        if (row[i] < min)
        {
            min = row[i];
        }
    }
    return min;
}

int main(int argc, char** argv)
{
    const int quantity_dims = 1;
    const int tag = 5;

    int rank, size;
    int source, destination; // отправитель получатель

    int dims[quantity_dims]{}, periods[quantity_dims]{}, coords[quantity_dims];// массивы для хранения размерности, периодичности и координат.

    int reorder = 0; // ранги могут быть перенумерованы(true) или нет(false). в нашем случае в новом коммуникаторе ранги всех процесов будут такими же

    MPI_Comm comm; // создаем переменную под новый коммуникатор
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (int i = 0; i < quantity_dims; i++)//задание размерности и переодичности
    {
        dims[i] = 0;
        periods[i] = 1;
    }

    MPI_Dims_create(size, quantity_dims, dims); // создание рамерности
    /*из методички:
    функция MPI_Dims_create позволяет выбрать сбалансированное
    распределение процессов по направлению координат, в зависимости от числа процессов в группе, и
    необязательных ограничений, которые могут быть определены пользователем

    Входные параметры:
        size - количество узлов в решетке;
        quantity_dims - размерность декартовой топологии.
    Выходные параметры:
        dims - целочисленный массив, определяющий количество узлов в каждой размерности.


    */

    MPI_Cart_create(MPI_COMM_WORLD, quantity_dims, dims, periods, reorder, &comm); // создаем коммуникатор
    /* Поподробнее:
    MPI_COMM_WORLD - входной (старый) коммуникатор;
    quantity_dims - количество измерений в декартовой топологии;
    dims - целочисленный массив размером ndims, определяющий количество процессов в каждом измерении;
    periods - массив размером ndims логических значений, определяющих периодичность (true) или нет (false) в каждом измерении;
    reorder	- ранги могут быть перенумерованы (true) или нет (false).
    &comm - новый коммуникатор
    */

    MPI_Cart_coords(comm, rank, quantity_dims, coords); // инициализируем координаты текущего процесса
    MPI_Cart_shift(comm, 0, -1, &source, &destination); //отвечает за топологию кольца
    //определение координаты соседних процессов и размером шага  смещения(положительным или отрицательным).

    int* rowA = new int[size];
    int* colB = new int[size];
    int* masArr = new int[size];

    createArr(rowA, size);
    printArr(rowA, size, "A", rank, "Row");
    createArr(colB, size);
    printArr(colB, size, "B", rank, "Col");

    for (int i = 0; i < size; i++)
    {
        int* mulMas = new int[size];
        int* maxKMas = new int[size];
        for (int j = 0; j < size; j++)
        {
            for (int k = 0; k < size; k++) {

                int temp = rowA[k] + colB[k];
                mulMas[k] = temp;
            }
            maxKMas[j] = findMaxElem(mulMas, size);
        }
        masArr[i] = findMaxElem(mulMas, size);
        if (i < size - 1) {
            MPI_Sendrecv_replace(colB, size, MPI_INT, destination, tag, source, tag, comm, &status); //для смещения данных вдоль направления какой либо координаты
        }
        delete[] mulMas;
        delete[] maxKMas;
    }

    int res = findMaxElem(masArr, size);
    cout << "Result " << rank << " : " << res << endl;

    delete[] rowA;
    delete[] colB;
    delete[] masArr;
    MPI_Comm_free(&comm);
    MPI_Finalize();
    return 0;
}