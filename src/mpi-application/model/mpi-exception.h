#ifndef NS3_MPI_APPLICATION_EXCEPTION_H
#define NS3_MPI_APPLICATION_EXCEPTION_H

#include <exception>
#include <string>

class MPIException : public std::exception {
private:
    std::string message;
public:
    explicit MPIException(const std::string &message) : message(message) {}

    explicit MPIException(std::string &&message) : message(message) {}

    explicit MPIException(const MPIException &) = default;

    explicit MPIException(MPIException &&) = default;

    MPIException &operator=(const MPIException &) = default;

    MPIException &operator=(MPIException &&) = default;

    const char *what() const noexcept override {
        return message.c_str();
    }
};

class MPISocketException : public MPIException {
public:
    explicit MPISocketException(const std::string &message) : MPIException(message) {}

    explicit MPISocketException(std::string &&message) : MPIException(message) {}

    explicit MPISocketException(const MPISocketException &) = default;

    explicit MPISocketException(MPISocketException &&exception) = default;

    MPISocketException &operator=(const MPISocketException &) = default;

    MPISocketException &operator=(MPISocketException &&) = default;
};

#endif //NS3_MPI_APPLICATION_EXCEPTION_H
