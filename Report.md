## **Report Questions**

---

### **Question 1 – Difference between `stat()` and `lstat()`**

Both `stat()` and `lstat()` are system calls used to get information about files, but the **main difference** is how they handle **symbolic links**.

| Function | Description |
|-----------|--------------|
| `stat(path, &buf)` | Returns information about the **file that the path points to**. If it’s a symbolic link, it follows the link and gives details about the **target file**. |
| `lstat(path, &buf)` | Returns information about the **link itself**, not the target file. So, if `path` is a symbolic link, it shows info about the **link** (like its size and permissions). |

#### 💡 Example
If you have a symbolic link `link.txt` pointing to `file.txt`:

- `stat("link.txt", &buf)` → info about `file.txt` (target)  
- `lstat("link.txt", &buf)` → info about `link.txt` (the link)

**In the `ls -l` command:**  
We use **`lstat()`** so that symbolic links are displayed with their own details, not the file they point to.

---

### **Question 2 – Using `st_mode` to Get File Type and Permissions**

The `st_mode` field in the `struct stat` (from `<sys/stat.h>`) stores **two kinds of information**:

1. **File Type** – what kind of file it is (regular file, directory, link, etc.)  
2. **Permissions** – read, write, execute permissions for owner, group, and others

---

### **1️⃣ Checking File Type**

You can find the file type by comparing `(mode & S_IFMT)` with specific macros:

| Macro | Meaning |
|--------|----------|
| `S_IFREG` | Regular file |
| `S_IFDIR` | Directory |
| `S_IFLNK` | Symbolic link |
| `S_IFCHR` | Character device |
| `S_IFBLK` | Block device |
| `S_IFIFO` | FIFO (pipe) |
| `S_IFSOCK` | Socket |

#### **Example Code:**
```c
#include <stdio.h>
#include <sys/stat.h>

void print_file_type(mode_t mode) {
    if (S_ISREG(mode))   printf("Regular file\n");
    else if (S_ISDIR(mode))  printf("Directory\n");
    else if (S_ISLNK(mode))  printf("Symbolic link\n");
    else if (S_ISCHR(mode))  printf("Character device\n");
    else if (S_ISBLK(mode))  printf("Block device\n");
    else if (S_ISFIFO(mode)) printf("FIFO/pipe\n");
    else if (S_ISSOCK(mode)) printf("Socket\n");
    else printf("Unknown type\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    struct stat info;
    if (stat(argv[1], &info) == -1) {
        perror("stat");
        return 1;
    }

    printf("File: %s\n", argv[1]);
    print_file_type(info.st_mode);

    return 0;
}
```

---

### **2️⃣ Checking Permissions**

Each permission is represented by a specific bit in `st_mode`:

| Symbol | Meaning | Bitmask |
|:--:|:--|:--|
| `r` | Read | `S_IRUSR`, `S_IRGRP`, `S_IROTH` |
| `w` | Write | `S_IWUSR`, `S_IWGRP`, `S_IWOTH` |
| `x` | Execute | `S_IXUSR`, `S_IXGRP`, `S_IXOTH` |

#### **Example (Printing Permissions):**
```c
printf("Owner: %c%c%c\n",
       (info.st_mode & S_IRUSR) ? 'r' : '-',
       (info.st_mode & S_IWUSR) ? 'w' : '-',
       (info.st_mode & S_IXUSR) ? 'x' : '-');
```

---

### ✅ **Summary**
- `stat()` → info about the **target file**.  
- `lstat()` → info about the **symbolic link itself**.  
- `st_mode` → contains **file type + permission bits**.  
- Use macros like `S_ISDIR()`, `S_IRUSR` to check type and permissions.


## **Feature:3**

## **Report Questions: Columnar Output and ioctl System Call**

---

### **1. Logic for Printing Items in a 'Down Then Across' Columnar Format**

When printing items (like filenames) in a columnar format similar to the Unix `ls` command, the goal is to display the list efficiently across multiple columns based on the terminal width.

#### **General Logic:**
1. **Determine the number of rows and columns:**
   - Find the **longest filename length**.
   - Use the **terminal width** to calculate how many columns can fit.
   - Compute the number of rows needed to display all items.

2. **Print items down each column:**
   - Unlike the usual row-major order (across first), we print **down each column** before moving to the next.
   - For example, if we have 10 filenames and 3 columns, we fill them as:
     ```
     file1   file4   file7
     file2   file5   file8
     file3   file6   file9
     ```

3. **Iterate carefully:**
   - A single loop (e.g., `for i in range(n)`) won’t work because it only processes items linearly, across one dimension.
   - You need **nested loops**: one for rows and one for columns, calculating the correct index (`index = row + column * rows`).

#### **Why a Single Loop is Insufficient:**
A simple loop prints filenames one after another (across a single line or column). It cannot manage:
- The layout structure (rows × columns)
- Proper alignment of filenames
- Wrapping based on terminal width

Therefore, **two-dimensional indexing** is required to print “down then across.”

---

### **2. Purpose of the `ioctl` System Call in This Context**

The `ioctl` system call is used to **query or control device parameters**. In this context (like in the `ls` implementation), it helps detect the **current terminal size** — specifically, the **number of columns (width)**.

#### **How It’s Used:**
```c
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

struct winsize w;
ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
printf("Terminal width: %d columns\n", w.ws_col);
```
- `STDOUT_FILENO`: the file descriptor for the terminal output
- `TIOCGWINSZ`: request code to get the window size
- `w.ws_col`: gives the number of columns in the terminal

This allows the program to automatically adjust column layout based on the current terminal size.

---

### **3. Limitations of Using a Fixed-Width Fallback (e.g., 80 Columns)**
If the program doesn’t use `ioctl` and instead assumes a **fixed width**, such as 80 columns:

- On **wider terminals**, output will waste space (few columns used).
- On **narrow terminals**, output may wrap or look misaligned.
- It won’t adapt if the user resizes the terminal window.

#### **Summary of Limitations:**
| Problem | Description |
|----------|--------------|
| Lack of adaptability | Layout doesn’t adjust to current screen size |
| Poor readability | Lines may wrap or appear uneven |
| Inefficient use of space | Wastes space on wide terminals |

Using `ioctl` provides a **dynamic and user-friendly layout** that matches the actual terminal dimensions, making output cleaner and more responsive.


## **Feature:4**

## **Report Questions: Display Mode Logic and Implementation Complexity**

---

### **1. Comparison: 'Down Then Across' (Vertical) vs 'Across' (Horizontal) Printing Logic**

When implementing a file listing program (like `ls`), the output can be formatted either **across** (row-wise) or **down then across** (column-wise). These two approaches differ significantly in implementation complexity.

#### **Horizontal ('Across') Printing Logic:**
- This is the **simpler** approach.
- The program prints filenames **sequentially** across the screen until it reaches the end of a row (based on terminal width), then moves to the next line.
- Only basic spacing and wrapping calculations are needed.

##### **Example (across):**
```
file1  file2  file3  file4
file5  file6  file7  file8
```

**Complexity:** Minimal. A single loop over filenames is enough.

---

#### **Vertical ('Down Then Across') Printing Logic:**
- This method prints items **down each column first**, then moves across to the next column.
- To do this, the program must **precompute**:
  - The **number of columns** that fit in the terminal.
  - The **number of rows** needed to display all items.
  - The **exact index** of each item in the list (`index = row + column * rows`).
- The logic requires **nested loops** and careful indexing to avoid missing or repeating entries.

##### **Example (down then across):**
```
file1  file4  file7
file2  file5  file8
file3  file6  file9
```

**Complexity:** Higher — requires more pre-calculation of column count, row count, and precise index mapping.

---

#### **Summary of Complexity Comparison:**
| Format | Implementation Complexity | Reason |
|---------|-----------------------------|---------|
| Across (Horizontal) | Low | Sequential printing using a single loop |
| Down then Across (Vertical) | High | Requires pre-calculation of rows, columns, and index mapping based on terminal width |

In short, **vertical printing** is more complex because it must organize filenames in a 2D grid before printing, while **horizontal printing** only needs linear iteration.

---

### **2. Strategy for Managing Display Modes (-l, -x, and Default)**

In a typical `ls`-like implementation, different display modes change how files are shown:

| Mode | Description |
|------|--------------|
| `-l` | Long listing (detailed file info like permissions, owner, size, date) |
| `-x` | Across (horizontal) column layout |
| Default | Down-then-across column layout |

#### **Strategy Used:**
1. **Parse command-line options** using `getopt()` or manual string checks.
2. **Store mode flags** (e.g., `flag_l`, `flag_x`, `flag_default`).
3. **Decide which print function to call** based on the active flag.

##### **Example Pseudocode:**
```c
if (flag_l)
    print_long_format(file_list);
else if (flag_x)
    print_across(file_list);
else
    print_down_across(file_list);
```

Each printing function handles formatting independently:
- `print_long_format()` → calls `lstat()` and displays permissions, size, owner, etc.
- `print_across()` → prints filenames row-wise (simple spacing logic).
- `print_down_across()` → prints filenames in vertical columns using precomputed layout.

This modular approach keeps each display mode **separate and maintainable**, while the main function only decides **which mode to trigger**.

---

### ✅ **Summary**
- The **'down then across'** logic is more complex because it needs **pre-calculation** of grid structure (rows, columns, and indices).
- The **'across'** logic is simpler — just a single loop with spacing.
- The program determines display mode (`-l`, `-x`, or default) through **flags**, and calls the corresponding **print function** to handle that format independently.


## **Feature:5**

## **Report Questions: Directory Reading and qsort Comparison Function**

---

### **1. Why Must All Directory Entries Be Read into Memory Before Sorting?**

When implementing file listing programs (like `ls`), it is necessary to **read all directory entries into memory** before sorting them. This is because sorting algorithms (like `qsort`) need access to the **entire dataset** to compare and order all items relative to one another.

#### **Reasoning:**
- Functions such as `readdir()` read directory entries **sequentially** from the filesystem.
- Once an entry is read, there is no direct way to “go back” to previous entries without re-opening and re-reading the directory.
- Sorting requires **random access** to all entries (for comparisons and swaps), which is only possible if they are stored in memory first.

#### **Typical Steps:**
1. Use `opendir()` to open the directory.
2. Use `readdir()` in a loop to read each entry.
3. Store all filenames (or `struct dirent` pointers) into an array.
4. Call `qsort()` to sort that array.
5. Print or process the sorted entries.

#### **Potential Drawbacks:**
For directories with **millions of files**, this approach can become inefficient:

| Issue | Description |
|--------|--------------|
| **High Memory Usage** | Each entry (name, metadata) consumes RAM. Millions of files may exceed available memory. |
| **Performance Overhead** | Copying and sorting a huge array takes time (O(n log n) complexity). |
| **Scalability Limits** | Programs may slow down or fail due to memory exhaustion on systems with limited resources. |

In short, reading all entries first is **necessary for sorting**, but **memory-intensive** for extremely large directories.

---

### **2. Purpose and Signature of the `qsort()` Comparison Function**

The C standard library function `qsort()` (from `<stdlib.h>`) is a **generic sorting function** that can sort any type of array.

#### **Function Signature:**
```c
void qsort(void *base, size_t nitems, size_t size,
            int (*compar)(const void *, const void *));
```

- **`base`** → Pointer to the first element of the array.
- **`nitems`** → Number of elements to sort.
- **`size`** → Size (in bytes) of each element.
- **`compar`** → Pointer to a comparison function that defines sorting order.

---

### **Comparison Function Details**
The comparison function tells `qsort()` **how to compare two elements**. It is called repeatedly by `qsort()` to decide ordering.

#### **Signature of the Comparison Function:**
```c
int compare(const void *a, const void *b);
```

#### **How It Works:**
- The function receives **two pointers** to the elements being compared.
- It must **cast** these `void*` pointers to the correct type (e.g., `char*`, `struct dirent*`).
- It returns:
  - `< 0` if `a` should come before `b`
  - `0` if they are equal
  - `> 0` if `a` should come after `b`

#### **Example: Sorting Filenames Alphabetically**
```c
int compare_names(const void *a, const void *b) {
    const char *name1 = *(const char **)a;
    const char *name2 = *(const char **)b;
    return strcmp(name1, name2);
}
```

This function can then be passed to `qsort()` like this:
```c
qsort(file_list, file_count, sizeof(char *), compare_names);
```

---

### **Why `const void *` Arguments Are Required**
- `qsort()` is **generic** — it can sort arrays of any data type.
- Using `const void *` allows it to accept and compare **any type of element**.
- The `const` ensures the comparison function **does not modify** the data while sorting.

This design provides **type flexibility and data safety**, making `qsort()` a universal sorting utility in C.

---

### ✅ **Summary**
- All directory entries must be read into memory to allow sorting algorithms like `qsort()` to access and rearrange them efficiently.
- This approach can be memory-heavy for very large directories.
- The `qsort()` function uses a comparison callback that accepts `const void *` arguments to support sorting of **any data type** safely and generically.


## **Feature:6**

# ANSI Escape Codes and Executable File Color Detection

## How ANSI Escape Codes Work

**ANSI escape codes** are special character sequences used to control formatting, color, and other output options in text terminals. They begin with the **escape character** (`\033` or `\x1B`) followed by `[` and a series of parameters ending with a letter that indicates the command.

For example, the general syntax for setting text color is:

```
\033[<style>;<text_color>;<background_color>m
```

Where:
- `<style>` sets attributes (e.g., `0` for reset, `1` for bold).
- `<text_color>` specifies the foreground color.
- `<background_color>` sets the background color (optional).

### Example: Printing Text in Green

To print text in **green**, you can use:

```c
printf("\033[32mThis text is green!\033[0m\n");
```

Explanation:
- `\033[32m` → Switches text color to green.
- `\033[0m` → Resets color and style back to default.

**Common color codes:**
| Color | Code |
|--------|------|
| Black | 30 |
| Red | 31 |
| Green | 32 |
| Yellow | 33 |
| Blue | 34 |
| Magenta | 35 |
| Cyan | 36 |
| White | 37 |

---

## Detecting Executable Files Using Permission Bits

In Linux, file permissions are stored in the `st_mode` field of the `struct stat` structure. You can check whether a file is **executable** using macros from `<sys/stat.h>`.

### Relevant Permission Bits:
- **Owner executable bit:** `S_IXUSR`
- **Group executable bit:** `S_IXGRP`
- **Others executable bit:** `S_IXOTH`

### Example Code:
```c
#include <sys/stat.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    struct stat st;
    if (stat(argv[1], &st) == -1) {
        perror("stat");
        return 1;
    }

    if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
        printf("\033[32m%s\033[0m\n", argv[1]); // Green if executable
    } else {
        printf("%s\n", argv[1]); // Default color
    }

    return 0;
}
```

### Explanation:
- The program uses `stat()` to retrieve file metadata.
- It checks if any executable bit (`S_IXUSR`, `S_IXGRP`, or `S_IXOTH`) is set.
- If yes → prints the filename in **green**.
- Otherwise → prints it normally.

---

### Summary
- **ANSI escape codes** modify terminal text color and style using sequences like `\033[32m`.
- **Executable detection** depends on checking the execute bits in `st_mode` using bitwise operations.
- Together, they enable color-coded listings similar to what the `ls` command does in Linux.


## **Feature:7**

# Report: Recursive `ls` Function Logic

## 1. What is a Base Case in Recursion?

A **base case** in a recursive function is the condition that **terminates the recursion**.  
It prevents the function from calling itself indefinitely, which would otherwise lead to an **infinite loop** and eventually a **stack overflow** error.

### Base Case in a Recursive `ls` Implementation
In a recursive version of the `ls` command, the function repeatedly lists directories and their subdirectories.  
The **base case** occurs when:
- The current directory contains **no subdirectories**, or  
- The entry being processed is either `"."` (current directory) or `".."` (parent directory).

These entries must be **skipped** to prevent infinite recursion into the same directories.

```c
if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
    continue; // Base case: stop recursion on these directories
```

This ensures that the recursion **stops naturally** when there are no further directories to process.

---

## 2. Why It’s Important to Construct a Full Path

When recursively calling the function to process subdirectories, it is essential to **build the complete path** — for example:

```
parent_dir/subdir
```

instead of just passing `"subdir"`.

### Reason:
Without the full path, the recursive function would look for `"subdir"` in the **current working directory**, not inside `"parent_dir"`.  
This would cause:
- **Incorrect traversal** (the function might open the wrong folder)
- **Errors** if `"subdir"` does not exist in the current directory
- **Incomplete or incorrect output**

### Correct Approach:
Construct the full path before making the recursive call:

```c
char path[1024];
snprintf(path, sizeof(path), "%s/%s", parent_dir, entry->d_name);
do_ls(path);
```

This ensures that:
- Each recursive call points to the **correct directory level**.
- The directory hierarchy is **preserved** and traversed properly.

---

## ✅ Summary

| Concept | Explanation |
|----------|--------------|
| **Base Case** | Stops recursion when no further subdirectories exist or when encountering `.` and `..`. Prevents infinite loops. |
| **Full Path Construction** | Ensures correct traversal by building `"parent_dir/subdir"` before recursive calls. Without it, the program may access wrong directories. |

By combining a proper **base case** and **full path construction**, the recursive `ls` implementation can safely and accurately traverse the entire directory tree.
