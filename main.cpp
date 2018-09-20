#include <iostream>
#include <cassert>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

class FileException : std::exception {};

#define PATH_INPUT   "/home/anton/text.txt"
#define PATH_OUTPUT1 "/home/anton/text1.txt"
#define PATH_OUTPUT2 "/home/anton/text2.txt"

class TextFile {

  /// Counts the file size
  /// \param fd File descriptor
  /// \return Size in bytes
  static size_t GetFileSize(int fd) {
    assert(fd != -1);
    struct stat st;
    if (fstat(fd, &st) == -1) {
      throw FileException();
    }
    return static_cast<size_t>(st.st_size);
  }

  /// Counts the number of lines in the text
  /// \param str Pointer to C-style string
  /// \return Number of '\n' occurrences + 1
  static size_t GetNumberOfLines(const char* str) {
    size_t count = 1;
    for (size_t i = 0; str[i] != 0; ++i) {
      if (str[i] == '\n') {
        ++count;
      }
    }
    return count;
  }

  /// Reads file from file descriptor
  /// \param fd File descriptor
  void ReadText(int fd) {
    text_size_ = GetFileSize(fd);
    text_      = new char[text_size_ + 1];

    size_t already_read = 0;
    ssize_t read_result;
    while ((read_result = read(fd, text_ + already_read, text_size_ - already_read)) > 0) {
      already_read += read_result;
    }

    if (read_result == -1) {
      throw FileException(); // TODO: ДОБАВИТЬ ОПИСАНИЕ ОШИБКИ : ERRNO + STRERROR
    }

    text_[text_size_] = 0;
  }

  void ScanLines() {
    number_of_lines_ = GetNumberOfLines(text_);
    lines_positions_ = new size_t[number_of_lines_];
    lines_lengths_   = new size_t[number_of_lines_];

    lines_positions_[0] = 0;

    size_t last_line_found = 0;
    for (size_t i = 0; i < text_size_; ++i) {
      if (text_[i] == '\n') {
        ++last_line_found;
        lines_positions_[last_line_found] = i + 1;
        lines_lengths_  [last_line_found - 1] = i - lines_positions_[last_line_found - 1];
        text_[i] = 0;
      }
    }
    lines_lengths_[number_of_lines_ - 1] = text_size_ - lines_positions_[number_of_lines_ - 1];
    assert(last_line_found + 1 == number_of_lines_);
  }

 public:
  TextFile() = delete;

  TextFile(char *path) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
      throw FileException(); // TODO: ДОБАВИТЬ ОПИСАНИЕ ОШИБКИ : ERRNO + STRERROR
    }

    ReadText(fd);
    ScanLines();
    close(fd);
  }

  ~TextFile() {
    delete[] lines_lengths_;
    delete[] lines_lengths_;
    delete[] text_;
  }

  /// Get pointer to line
  /// \param i Number of line
  /// \return Pointer to the line
  const char* GetLine(size_t i) const {
    assert(i < number_of_lines_);
    return text_ + lines_positions_[i];
  }

  /// Counts the lengths of the line
  /// \param i Number of the line
  /// \return Length of the line
  size_t GetLineLength(size_t i) const {
    assert(i < text_size_);
    return lines_lengths_[i];
  }

  /// Prints the text to standart output
  void PrintText() const {
    for (size_t i = 0; i < number_of_lines_; ++i) {
      printf("%s\n", GetLine(i));
    }
  }

  /// Swaps the lines
  /// \param i Index of the first line to swap
  /// \param j Index of the second lines to swap
  void SwapLines(size_t i, size_t j) {
    assert(i < number_of_lines_);
    assert(j < number_of_lines_);
    std::swap(lines_positions_[i], lines_positions_[j]);
    std::swap(lines_lengths_[i], lines_lengths_[j]);
  }

  /// Sorts the lines
  /// \param cmp Comparator for C-style strings, s.t. cmp(str1, str2, length1, length2) returns True iff str1 is less than str2, where str1 and str2 point to string beginnings, length1 and length2 are their lengths
  void Sort(bool (*cmp)(const char*, const char*, size_t, size_t)) {
    for (size_t i = 0; i < number_of_lines_; ++i) {
      for (size_t j = i + 1; j < number_of_lines_; ++j) {
        if (!cmp(GetLine(i), GetLine(j), GetLineLength(i), GetLineLength(j))) {
          SwapLines(i, j);
        }
      }
    }
  }

  void PrintLine(size_t i, int fd) const {
    assert(i < number_of_lines_);

    const char* ptr          = GetLine(i);
    size_t line_length = GetLineLength(i);

    int already_write = 0;
    ssize_t write_result;

    while (already_write < line_length) {
      write_result = write(fd, ptr, line_length);
      if (write_result == -1) {
        throw FileException();
      }

      already_write += write_result;
      ptr           += write_result;
    }
  }

  void WriteToFile(const char* path) {
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
      throw FileException();
    }

    for (size_t i = 0; i < number_of_lines_; ++i) {
      PrintLine(i, fd);
      if (write(fd, "\n", 1) != 1) {
        throw FileException();
      }
    }

    close(fd);
  }

 private:
  size_t  number_of_lines_;
  size_t  text_size_;

  size_t* lines_positions_{nullptr};
  size_t* lines_lengths_{nullptr};
  char*   text_{nullptr};

};

class TaskSolver {

  static bool Compare(const char* str1, const char* str2, size_t length1, size_t length2) {
    size_t i;
    for (i = 0; str1[i] != 0 && str2[i] != 0; ++i) {
      if (str1[i] < str2[i]) {
        return true;
      }
      if (str2[i] < str1[i]) {
        return false;
      }
    }

    return length1 < length2;
  }

  static bool CompareReversed(const char* str1, const char* str2, size_t length1, size_t length2) {
    if (!length1 || !length2) {
      return length1 == 0;
    }

    str1 = str1 + length1 - 1;
    str2 = str2 + length2 - 1;

    int i, j;
    for (i = 0, j = 0; i < length1 && j < length2;) {

      if (!isalpha(str1[-i])) {
        ++i;
        continue;
      }

      if (!isalpha(str2[-j])) {
        ++j;
        continue;
      }

      if (str1[-i] < str2[-j]) {
        return true;
      }
      if (str2[-j] < str1[-i]) {
        return false;
      }
      ++i;
      ++j;
    }

    return length1 < length2;
  }

 public:

  TaskSolver() = delete;

  explicit TaskSolver(char* path) : text_file_(path) {
    text_file_.Sort(Compare);
    text_file_.WriteToFile(PATH_OUTPUT1);

    text_file_.Sort(CompareReversed);
    text_file_.WriteToFile(PATH_OUTPUT2);
  }

  ~TaskSolver() = default;

 private:

  TextFile text_file_;

};

int main() {
  TaskSolver task_solver(PATH_INPUT);
  return 0;
}
