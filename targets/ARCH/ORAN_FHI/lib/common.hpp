/*******************************************************************************
 *
 * <COPYRIGHT_TAG>
 *
 *******************************************************************************/

/* This is the new utility file for all tests, all new common functionality has to go here.
   When contributing to the common.hpp please focus on readability and maintainability rather than
   execution time. */
#ifndef XRANLIB_COMMON_HPP
#define XRANLIB_COMMON_HPP

/* Disable warnings generated by JSON parser */
#pragma warning(disable : 191)
#pragma warning(disable : 186)
#pragma warning(disable : 192)

#include <exception>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <immintrin.h>
#include <malloc.h>

#define _BBLIB_DPDK_

#ifdef _BBLIB_DPDK_
#include <rte_config.h>
#include <rte_malloc.h>
#endif

//#include "gtest/gtest.h"

#include "common_typedef_xran.h"

#include "json.hpp"

using json = nlohmann::json;

#define ASSERT_ARRAY_NEAR(reference, actual, size, precision) \
        assert_array_near(reference, actual, size, precision)

#define ASSERT_ARRAY_EQ(reference, actual, size) \
        assert_array_eq(reference, actual, size)

#define ASSERT_AVG_GREATER_COMPLEX(reference, actual, size, precision) \
        assert_avg_greater_complex(reference, actual, size, precision)

struct BenchmarkParameters
{
    static long repetition;
    static long loop;
    static unsigned cpu_id;
};

struct missing_config_file_exception : public std::exception
{
    const char * what () const throw () override {
        return "JSON file cannot be opened!";
    }
};

struct reading_input_file_exception : public std::exception
{
    const char * what () const throw () override {
        return "Input file cannot be read!";
    }
};

/*!
    \brief Attach current process to the selected core.
    \param [in] cpu Core number.
    \return 0 on success, -1 otherwise.
*/
int bind_to_cpu(const unsigned cpu);

/*!
    \brief Calculate the mean and variance from the result of the run_benchmark.
    \param [in] values Vector with result values.
    \return std::pair where the first element is mean and the second one is standard deviation.
    \note It's not a general mean/stddev function it only works properly when feed with data from
          the benchmark function.
*/
std::pair<double, double> calculate_statistics(const std::vector<long> values);

/*!
    \brief For a given number return sequence of number from 0 to number - 1.
    \param [in] number Positive integer value.
    \return Vector with the sorted integer numbers between 0 and number - 1.
*/
std::vector<unsigned> get_sequence(const unsigned number);

/*!
    \brief Read JSON from the given file.
    \param [in] filename name of the .json file.
    \return JSON object with data.
    \throws missing_config_file_exception when file cannot be opened.
*/
json read_json_from_file(const std::string &filename);

/*!
    \brief Read binary data from the file.
    \param [in] filename name of the binary file.
    \return Pointer to the allocated memory with data from the file.
    \throws std::runtime_error when memory cannot be allocated.
*/
//char* read_data_to_aligned_array(const std::string &filename);

/*!
    \brief Measure the TSC on the machine
    \return Number of ticks per us
*/
unsigned long tsc_recovery();

/*!
    \brief Return the current value of the TSC
    \return Current TSC value
*/
unsigned long tsc_tick();

/*!
    \class KernelTests

    Each test class has to inherit from KernelTests class as it provides GTest support and does a lot
    of setup (including JSON) an provides useful methods to operate on loaded JSON file.
    Unfortunately GTest is limited in the way that all TEST_P within the class are called for all
    cases/parameters, but we usually want two different data sets for functional and performance
    tests (or maybe other types of tests). Because of that to use different data sets we need to
    create separate classes, hence performance and functional test are in separate classes. it adds
    an extra overhead, but adds much more flexibility. init_test(...) is used to select data set from
    the JSON file.

    Important note on the JSON file structure. Top JSON object can have as many section (JSON
    objects) as needed, but each have to have a distinct name that is used by init_test. Then
    each section must contain an array of objects (test cases) where each object has a name,
    parameters and references. Everything inside parameters and references can be completely custom
    as it's loaded by get_input/reference_parameter function. JSON values can be either literal
    values, e.g. 1, 0.001, 5e-05, etc. or filename. Depends on the get type test framework can either
    read the value or load data from the file - and it happens automatically (*pff* MAGIC!).
*/
#if 0 //Ann: we don't need this test in OAI, so we comment out all GTest dependencies to include xran_lib_wrap.hpp
class KernelTests : public testing::TestWithParam<unsigned>
{
public:
    static json conf;
    static std::string test_type;

    static void SetUpTestCase()
    {
        test_type = "None";

        try
        {
            conf = read_json_from_file("conf.json");
        }
        catch(missing_config_file_exception &e)
        {
            std::cout << "[----------] SetUpTestCase failed: " << e.what() << std::endl;
            exit(-1);
        }

        tsc = tsc_recovery();

        if(!tsc)
        {
            std::cout << "[----------] SetUpTestCase failed: TSC recovery failed" << std::endl;
            exit(-1);
        }
    }

    static void TearDownTestCase()
    {
        /* Free resources - nothing to free at the moment */
    }

    static unsigned get_number_of_cases(const std::string &type)
    {
        try
        {
            json json_data = read_json_from_file("conf.json");

            return json_data[type].size();
        }
        catch(missing_config_file_exception &e)
        {
            std::cout << "[----------] get_number_of_cases failed: " << e.what() << std::endl;

            exit(-1);
        }
        catch(std::domain_error &e)
        {
            std::cout << "[----------] get_number_of_cases failed: " << e.what() << std::endl;
            std::cout << "[----------] Use a default value: 0" << std::endl;

            return 0;
        }
    }

protected:
    double division_factor = 1.0;
    std::string result_units = "None";
    int parallelization_factor = 1;

    /*!
        \brief Set division factor
        \param [in] factor Division factor that divides mean and standard deviation.
    */
    void set_division_factor(const double factor)
    {
        division_factor = factor;
    }

    /*!
        \brief Set reults units
        \param [in] units Units that are displayed in the report.
    */
    void set_results_units(const std::string &units)
    {
        result_units = units;
    }

    /*!
        \brief Set size of processed data
        \param [in] size Size of processed data used to calculate module throughput.
    */
    void set_parallelization_factor(const int factor)
    {
        parallelization_factor = factor;
    }

    /*!
        \brief Run performance test case for a given function.
        \param [in] isa Used Instruction Set.
        \param [in] module_name name of the tested kernel.
        \param [in] function function to be tested.
        \param [in] args function's arguments.
    */
    template <typename F, typename ... Args>
    void performance(const std::string &isa, const std::string &module_name, F function,
                     Args ... args) {
        ASSERT_EQ(0, bind_to_cpu(BenchmarkParameters::cpu_id)) << "Failed to bind to cpu!";

        const auto result = run_benchmark(function, args ...);
        const auto scaled_mean = result.first / division_factor / tsc;
        const auto scaled_stddev = result.second / division_factor / tsc;

        print_and_store_results(isa, get_case_name(), module_name, get_case_name(), result_units,
                                parallelization_factor, scaled_mean, scaled_stddev);
    }

    /*!
        \brief Print unique test description to the results xml file
        \param [in] isa Used Instruction Set.
        \param [in] module_name name of the tested kernel.
        \param [in] function function to be tested.
    */
    void print_test_description(const std::string &isa, const std::string &module_name) {
        print_and_store_results(isa, get_case_name(), module_name, get_case_name(), result_units,
                                parallelization_factor, 0, 0);
    }

    //! @{
    /*!
        \brief Load selected data from a JSON object.
         get_input_parameter loads data from parameters section of the test case in JSON file and
         get_reference_parameter does the same thing for references section.

         Get parameter function uses template type to figure out how to load parameters. If type
         is NOT a pointer it'll load value directly from the JSON. Otherwise path to the test
         vector is expected and function will allocate memory, load data from the binary file to
         this memory location and return pointer to it. For example in here we request to load
         pointer to float so llrs filed is expected to be
         a path to the binary file.
    */
    template <typename T>
    T get_input_parameter(const std::string &parameter_name)
    {
        try
        {
            return get_parameter<T>("parameters", parameter_name);
        }
        catch (std::domain_error &e)
        {
            std::cout << "[----------] get_input_parameter (" << parameter_name
                      << ") failed: " << e.what()
                      << ". Did you mispell the parameter name?" << std::endl;
            throw;
        }
        catch(reading_input_file_exception &e)
        {
            std::cout << "[----------] get_input_parameter (" << parameter_name
                      << ") failed: " << e.what() << std::endl;
            throw;
        }
    }

    template <typename T>
    T get_input_parameter(const std::string &subsection_name, const std::string &parameter_name)
    {
        try
        {
            return get_parameter<T>("parameters", subsection_name, parameter_name);
        }
        catch (std::domain_error &e)
        {
            std::cout << "[----------] get_input_parameter (" << subsection_name << "." << parameter_name
                      << ") failed: " << e.what()
                      << ". Did you mispell the parameter name?" << std::endl;
            throw;
        }
        catch(reading_input_file_exception &e)
        {
            std::cout << "[----------] get_input_parameter (" << subsection_name << "." << parameter_name
                      << ") failed: " << e.what() << std::endl;
            throw;
        }
    }
    
    template <typename T>
    T get_input_parameter(const std::string &subsection_name, const int index, const std::string &parameter_name)
    {
        try
        {
            return get_parameter<T>("parameters", subsection_name, index, parameter_name);
        }
        catch (std::domain_error &e)
        {
            std::cout << "[----------] get_input_parameter (" << subsection_name << "[" << index << "]." << parameter_name
                      << ") failed: " << e.what()
                      << ". Did you mispell the parameter name?" << std::endl;
            throw;
        }
        catch(reading_input_file_exception &e)
        {
            std::cout << "[----------] get_input_parameter (" << subsection_name << "[" << index << "]." << parameter_name
                      << ") failed: " << e.what() << std::endl;
            throw;
        }
    }
    int get_input_parameter_size(const std::string &subsection_name, const std::string &parameter_name)
    {
        try
        {
            auto array_size = conf[test_type][GetParam()]["parameters"][subsection_name][parameter_name].size();
            return (array_size);
        }
        catch (std::domain_error &e)
        {
            std::cout << "[----------] get_input_parameter_size (" << subsection_name << "." << parameter_name
                      << ") failed: " << e.what()
                      << ". Did you mispell the parameter name?" << std::endl;
            return (-1);
        }
        catch(reading_input_file_exception &e)
        {
            std::cout << "[----------] get_input_parameter_size (" << subsection_name << "." << parameter_name
                      << ") failed: " << e.what() << std::endl;
            throw;
        }
    }
    int get_input_subsection_size(const std::string &subsection_name)
    {
        try
        {
            auto array_size = conf[test_type][GetParam()]["parameters"][subsection_name].size();
            return (array_size);
        }
        catch (std::domain_error &e)
        {
            std::cout << "[----------] get_input_subsection_size (" << subsection_name 
                      << ") failed: " << e.what()
                      << ". Did you mispell the subsection name?" << std::endl;
            return (-1);
        }
        catch(reading_input_file_exception &e)
        {
            std::cout << "[----------] get_input_subsection_size (" << subsection_name
                      << ") failed: " << e.what() << std::endl;
            throw;
        }
    }

    template <typename T>
    T get_reference_parameter(const std::string &parameter_name)
    {
        try
        {
            return get_parameter<T>("references", parameter_name);
        }
        catch (std::domain_error &e)
        {
            std::cout << "[----------] get_reference_parameter (" << parameter_name
                      << ") failed: " << e.what()
                      << ". Did you mispell the parameter name?" << std::endl;
            throw;
        }
        catch(reading_input_file_exception &e)
        {
            std::cout << "[----------] get_reference_parameter (" << parameter_name
                      << ") failed: " << e.what() << std::endl;
            throw;
        }
    }
    //! @}

    /*!
        \brief Get name of the test case from JSON file.
        \return Test'ss case name or a default name if name field is missing.
    */
    const std::string get_case_name()
    {
        try
        {
            return conf[test_type][GetParam()]["name"];
        }
        catch (std::domain_error &e)
        {
            std::cout << "[----------] get_case_name failed: " << e.what()
                      << ". Did you specify a test name in JSON?" << std::endl;
            std::cout << "[----------] Using a default name instead" << std::endl;

            return "Default test name";
        }
    }

    /*!
        \brief Defines section in the conf.json that is used to load parameters from.
        \param [in] type Name of the section in the JSON file.
    */
    void init_test(const std::string &type)
    {
        test_type = type;
        const std::string name = get_case_name();
        std::cout << "[----------] Test case: " << name << std::endl;
    }

private:
    static unsigned long tsc;

    template<typename T>
    struct data_reader {
        static T read_parameter(const int index, const std::string &type,
                                const std::string &parameter_name)
        {
            return conf[test_type][index][type][parameter_name];
        }
    };

    template<typename T>
    struct data_reader<std::vector<T>> {
        static std::vector<T> read_parameter(const int index, const std::string &type,
                                             const std::string &parameter_name)
        {
            auto array_size = conf[test_type][index][type][parameter_name].size();

            std::vector<T> result(array_size);

            for(unsigned number = 0; number < array_size; number++)
                result.at(number) = conf[test_type][index][type][parameter_name][number];

            return result;
        }
    };

    template<typename T>
    struct data_reader<T*> {
        static T* read_parameter(const int index, const std::string &type,
                                 const std::string &parameter_name)
        {
            return (T*) read_data_to_aligned_array(conf[test_type][index][type][parameter_name]);
        }
    };

    template <typename T>
    T get_parameter(const std::string &type, const std::string &parameter_name)
    {
        return data_reader<T>::read_parameter(GetParam(), type, parameter_name);
    }

    template<typename T>
    struct data_reader2 {
        static T read_parameter(const int index, const std::string &type,
                                const std::string &subsection_name,
                                const std::string &parameter_name)
        {
            return conf[test_type][index][type][subsection_name][parameter_name];
        }
    };

    template<typename T>
    struct data_reader2<std::vector<T>> {
        static std::vector<T> read_parameter(const int index, const std::string &type,
                                             const std::string &subsection_name,
                                             const std::string &parameter_name)
        {
            auto array_size = conf[test_type][index][type][subsection_name][parameter_name].size();

            std::vector<T> result(array_size);

            for(unsigned number = 0; number < array_size; number++)
                result.at(number) = conf[test_type][index][type][subsection_name][parameter_name][number];

            return result;
        }
    };

    template<typename T>
    struct data_reader2<T*> {
        static T* read_parameter(const int index, const std::string &type,
                                 const std::string &subsection_name,
                                 const std::string &parameter_name)
        {
            return (T*) read_data_to_aligned_array(conf[test_type][index][type][subsection_name][parameter_name]);
        }
    };
    template <typename T>
    T get_parameter(const std::string &type, const std::string &subsection_name, const std::string &parameter_name)
    {
        return data_reader2<T>::read_parameter(GetParam(), type, subsection_name, parameter_name);
    }

    template<typename T>
    struct data_reader3 {
        static T read_parameter(const int index, const std::string &type,
                                const std::string &subsection_name,
                                const int subindex,
                                const std::string &parameter_name)
        {
            return conf[test_type][index][type][subsection_name][subindex][parameter_name];
        }
    };

    template<typename T>
    struct data_reader3<std::vector<T>> {
        static std::vector<T> read_parameter(const int index, const std::string &type,
                                             const std::string &subsection_name,
                                             const int subindex,
                                             const std::string &parameter_name)
        {
            auto array_size = conf[test_type][index][type][subsection_name][subindex][parameter_name].size();

            std::vector<T> result(array_size);

            for(unsigned number = 0; number < array_size; number++)
                result.at(number) = conf[test_type][index][type][subsection_name][subindex][parameter_name][number];

            return result;
        }
    };

    template<typename T>
    struct data_reader3<T*> {
        static T* read_parameter(const int index, const std::string &type,
                                 const std::string &subsection_name,
                                 const int subindex,
                                 const std::string &parameter_name)
        {
            return (T*) read_data_to_aligned_array(conf[test_type][index][type][subsection_name][subindex][parameter_name]);
        }
    };
    template <typename T>
    T get_parameter(const std::string &type, const std::string &subsection_name, const int subindex, const std::string &parameter_name)
    {
        return data_reader3<T>::read_parameter(GetParam(), type, subsection_name, subindex, parameter_name);
    }

    void print_and_store_results(const std::string &isa,
                                 const std::string &parameters,
                                 const std::string &module_name,
                                 const std::string &test_name,
                                 const std::string &unit,
                                 const int para_factor,
                                 const double mean,
                                 const double stddev);
};

/*!
    \brief Run the given function and return the mean run time and stddev.
    \param [in] function Function to benchmark.
    \param [in] args Function's arguments.
    \return std::pair where the first element is mean and the second one is standard deviation.
*/
template <typename F, typename ... Args>
std::pair<double, double> run_benchmark(F function, Args ... args)
{
    std::vector<long> results((unsigned long) BenchmarkParameters::repetition);

    for(unsigned int outer_loop = 0; outer_loop < BenchmarkParameters::repetition; outer_loop++) {
        const auto start_time =  __rdtsc();
        for (unsigned int inner_loop = 0; inner_loop < BenchmarkParameters::loop; inner_loop++) {
                function(args ...);
        }
        const auto end_time = __rdtsc();
        results.push_back(end_time - start_time);
     }

    return calculate_statistics(results);
};

/*!
    \brief Assert elements of two arrays. It calls ASSERT_EQ for each element of the array.
    \param [in] reference Array with reference values.
    \param [in] actual Array with the actual output.
    \param [in] size Size of the array.
*/
template <typename T>
void assert_array_eq(const T* reference, const T* actual, const int size)
{
    for(int index = 0; index < size ; index++)
    {
        ASSERT_EQ(reference[index], actual[index])
                          <<"The wrong number is index: "<< index;
    }
}

/*!
    \brief Assert elements of two arrays. It calls ASSERT_NEAR for each element of the array.
    \param [in] reference Array with reference values.
    \param [in] actual Array with the actual output.
    \param [in] size Size of the array.
    \param [in] precision Precision fo the comparision used by ASSERT_NEAR.
*/
template <typename T>
void assert_array_near(const T* reference, const T* actual, const int size, const double precision)
{
    for(int index = 0; index < size ; index++)
    {
        ASSERT_NEAR(reference[index], actual[index], precision)
                                <<"The wrong number is index: "<< index;
    }
}

template <>
void assert_array_near<complex_float>(const complex_float* reference, const complex_float* actual, const int size, const double precision)
{
    for(int index = 0; index < size ; index++)
    {
        ASSERT_NEAR(reference[index].re, actual[index].re, precision)
                             <<"The wrong number is RE, index: "<< index;
        ASSERT_NEAR(reference[index].im, actual[index].im, precision)
                             <<"The wrong number is IM, index: "<< index;
    }
}

/*!
    \brief Assert average diff of two arrays. It calls ASSERT_GT to check the average.
    \param [in] reference Array with reference values, interleaved IQ inputs.
    \param [in] actual Array with the actual output, interleaved IQ inputs.
    \param [in] size Size of the array, based on complex inputs.
    \param [in] precision Precision for the comparison used by ASSERT_GT.
*/
template<typename T>
void assert_avg_greater_complex(const T* reference, const T* actual, const int size, const double precision)
{
    float mseDB, MSE;
    double avgMSEDB = 0.0;
    for (int index = 0; index < size; index++) {
        T refReal = reference[2*index];
        T refImag = reference[(2*index)+1];
        T resReal = actual[2*index];
        T resImag = actual[(2*index)+1];

        T errReal = resReal - refReal;
        T errIm = resImag - refImag;

         For some unit tests, e.g. PUCCH deomdulation, the expected output is 0. To avoid a
           divide by zero error, check the reference results to determine if the expected result
           is 0 and, if so, add a 1 to the division. 
        if (refReal == 0 && refImag == 0)
            MSE = (float)(errReal*errReal + errIm*errIm)/(float)(refReal*refReal + refImag*refImag + 1);
        else
            MSE = (float)(errReal*errReal + errIm*errIm)/(float)(refReal*refReal + refImag*refImag);

        if(MSE == 0)
            mseDB = (float)(-100.0);
        else
            mseDB = (float)(10.0) * (float)log10(MSE);

        avgMSEDB += (double)mseDB;
        }

        avgMSEDB /= size;

        ASSERT_GT(precision, avgMSEDB);
}

/*!
    \brief Allocates memory of the given size.

    aligned_malloc is wrapper to functions that allocate memory:
    'rte_malloc' from DPDK if hugepages are defined, 'memalign' otherwise.
    Size is defined as a number of variables of given type e.g. floats, rather than bytes.
    It hides sizeof(T) multiplication and cast hence makes things cleaner.

    \param [in] size Size of the memory to allocate.
    \param [in] alignment Bytes alignment of the allocated memory. If 0, the return is a pointer
                that is suitably aligned for any kind of variable (in the same manner as malloc()).
                Otherwise, the return is a pointer that is a multiple of align. In this case,
                it must be a power of two. (Minimum alignment is the cacheline size, i.e. 64-bytes)
    \return Pointer to the allocated memory.
*/
template <typename T>
T* aligned_malloc(const int size, const unsigned alignment)
{
#ifdef _BBLIB_DPDK_
    return (T*) rte_malloc(NULL, sizeof(T) * size, alignment);
#else
#ifndef _WIN64
    return (T*) memalign(alignment, sizeof(T) * size);
#else
    return (T*)_aligned_malloc(sizeof(T)*size, alignment);
#endif
#endif
}

/*!
    \brief Frees memory pointed by the given pointer.

    aligned_free is a wrapper for functions that free memory allocated by
    aligned_malloc: 'rte_free' from DPDK if hugepages are defined and 'free' otherwise.

    \param [in] ptr Pointer to the allocated memory.
*/
template <typename T>
void aligned_free(T* ptr)
{
#ifdef _BBLIB_DPDK_
    rte_free((void*)ptr);
#else

#ifndef _WIN64
    free((void*)ptr);
#else
    _aligned_free((void *)ptr);
#endif
#endif
}

/*!
    \brief generate random numbers.

    It allocates memory and populate it with random numbers using C++11 default engine and
    uniform real / int distribution (where lo_range <= x <up_range). Don't forget to free
    allocated memory!

    \param [in] size Size of the memory to be filled with random data.
    \param [in] alignment Bytes alignment of the memory.
    \param [in] distribution Distribuiton for random generator.
    \return Pointer to the allocated memory with random data.
*/
template <typename T, typename U>
T* generate_random_numbers(const long size, const unsigned alignment, U& distribution)
{
    auto array = (T*) aligned_malloc<char>(size * sizeof(T), alignment);

    std::random_device random_device;
    std::default_random_engine generator(random_device());

    for(long i = 0; i < size; i++)
        array[i] = (T)distribution(generator);

    return array;
}

/*!
    \brief generate random data.

    It allocates memory and populate it with random data using C++11 default engine and
    uniform integer distribution (bytes not floats are uniformly distributed). Don't forget
    to free allocated memory!

    \param [in] size Size of the memory to be filled with random data.
    \param [in] alignment Bytes alignment of the memory.
    \return Pointer to the allocated memory with random data.
*/
template <typename T>
T* generate_random_data(const long size, const unsigned alignment)
{
    std::uniform_int_distribution<> random(0, 255);

    return (T*)generate_random_numbers<char, std::uniform_int_distribution<>>(size * sizeof(T), alignment, random);
}

/*!
    \brief generate integer random numbers.

    It allocates memory and populate it with random numbers using C++11 default engine and
    uniform integer distribution (where lo_range <= x < up_range). Don't forget
    to free allocated memory! The result type generated by the generator should be one of
    int types.

    \param [in] size Size of the memory to be filled with random data.
    \param [in] alignment Bytes alignment of the memory.
    \param [in] lo_range Lower bound of range of values returned by random generator.
    \param [in] up_range Upper bound of range of values returned by random generator.
    \return Pointer to the allocated memory with random data.
*/
template <typename T>
T* generate_random_int_numbers(const long size, const unsigned alignment, const T lo_range,
                               const T up_range)
{
    std::uniform_int_distribution<T> random(lo_range, up_range);

    return generate_random_numbers<T, std::uniform_int_distribution<T>>(size, alignment, random);
}

/*!
    \brief generate real random numbers.

    It allocates memory and populate it with random numbers using C++11 default engine and
    uniform real distribution (where lo_range <= x <up_range). Don't forget to free
    allocated memory! The result type generated by the generator should be one of
    real types: float, double or long double.

    \param [in] size Size of the memory to be filled with random data.
    \param [in] alignment Bytes alignment of the memory.
    \param [in] lo_range Lower bound of range of values returned by random generator.
    \param [in] up_range Upper bound of range of values returned by random generator.
    \return Pointer to the allocated memory with random data.
*/
template <typename T>
T* generate_random_real_numbers(const long size, const unsigned alignment, const T lo_range,
                                const T up_range)
{
    std::uniform_real_distribution<T> distribution(lo_range, up_range);

    return generate_random_numbers<T, std::uniform_real_distribution<T>>(size, alignment, distribution);
}
#endif

#endif //XRANLIB_COMMON_HPP
