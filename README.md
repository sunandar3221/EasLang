# EasLang (.eas)

EasLang adalah bahasa pemrograman modern berperforma tinggi dengan filosofi sintaks ultra-bersih (ultra-clean syntax), manajemen memori deterministik tanpa Garbage Collector (Zero-Cost RAII), arsitektur dual-engine (Bytecode Virtual Machine & AOT Compiler), serta standard library bawaan yang lengkap (*batteries-included*).

Proyek ini dilisensikan di bawah **MIT License**.

---

## Daftar Isi
1. [Filosofi & Arsitektur](#1-filosofi--arsitektur)
2. [Kursus Kilat EasLang (Crash Course)](#2-kursus-kilat-easlang-crash-course)
3. [Kamus Keyword & Operator](#3-kamus-keyword--operator)
4. [Hasil Benchmark & Komparasi Kecepatan](#4-hasil-benchmark--komparasi-kecepatan)
5. [Panduan Build & Eksekusi](#5-panduan-build--eksekusi)
6. [Lisensi](#6-lisensi)

---

## 1. Filosofi & Arsitektur

- **Ultra-Clean Syntax**:
  - Bebas dari kurung kurawal `{}`
  - Bebas dari titik dua `:`
  - Bebas dari titik koma `;`
  - Bebas dari keyword `return` (ekspresi terakhir yang dievaluasi otomatis menjadi nilai balik fungsi)
  - Tanpa deklarasi tipe data wajib yang bertele-tele
  - Blok kode berbasis baris dan indentasi alami (dengan opsi penutup blok `end`)
- **Arsitektur Zero-Disk-Cache & Dual-Engine**:
  - **In-Memory Flat-Stack Execution (Default CLI)**: Menjalankan skrip `.eas` langsung secara murni di dalam memori tanpa membuat berkas cache di disk (*zero storage bloat*). Menghasilkan eksekusi instan (0 ms compile time) dengan dispatch opcodes dan in-place register updates yang sangat cepat.
  - **Ahead-Of-Time (AOT) Compiler Backend (`eas build`)**: Mengompilasi skrip `.eas` langsung menjadi berkas binary `.exe` native mandiri menggunakan optimasi agresif `-O3 -march=native -flto` tanpa dependensi runtime.
- **Manajemen Memori Deterministik (Zero GC Overhead)**:
  - Menggunakan model Zero-Cost RAII sehingga alokasi dan dealokasi memori terjadi seketika tanpa jeda *stop-the-world garbage collection*.
- **Batteries-Included Standard Library**:
  - File I/O bawaan (`read`, `write`)
  - Jaringan dan HTTP Client (`get`, `send`)
  - Objek, map, dan list dinamis
  - Scaffolding aplikasi desktop native Win32 (`app`, `window`, `run`)

---

## 2. Kursus Kilat EasLang (Crash Course)

### Bab 1: Output Pertama & Variabel
Di EasLang, cukup tulis nama variabel dan nilainya tanpa tipe data dan tanpa titik koma. Output ditampilkan menggunakan keyword `print`.

```eas
name = "Budi"
age = 20
print name
print age
```

### Bab 2: Operasi Aritmatika & Ekspresi
EasLang mendukung operator standar `+`, `-`, `*`, `/`, `%` dengan prioritas matematis yang benar serta pengelompokan menggunakan tanda kurung `()`.

```eas
val1 = 100
val2 = 25
sum = val1 + val2
diff = val1 - val2
prod = val1 * val2
quot = val1 / val2
print sum
print diff
print prod
print quot
```

### Bab 3: List / Array Dinamis
List dideklarasikan dengan kurung siku `[]` dan diakses menggunakan indeks berbasis nol `[i]`.

```eas
names = ["Budi", "Andi", "Sari"]
print names[0]
print names[1]
print names[2]
```

### Bab 4: Percabangan (`if` - `else`)
Percabangan menggunakan kata kunci `if` dan `else` berbasis indentasi tanpa tanda kurung kurawal atau titik dua. Keyword `end` dapat digunakan secara opsional.

```eas
age = 20
if age >= 18
    print "Adult"
else
    print "Minor"
```

### Bab 5: Perulangan (`loop` & `while`)
- Gunakan `loop <jumlah>` untuk mengulang blok sebanyak $n$ kali.
- Gunakan `while <kondisi>` untuk perulangan berbasis evaluasi kondisi.

```eas
loop 3
    print "Pengulangan loop"

counter = 5
while counter > 0
    print counter
    counter = counter - 1
```

### Bab 6: Fungsi & Implicit Return (`fn`)
Fungsi dideklarasikan dengan kata kunci `fn`, diikuti nama fungsi dan parameter yang dipisahkan spasi. **Tidak diperlukan keyword `return`**: baris atau ekspresi terakhir dalam fungsi otomatis menjadi nilai baliknya.

```eas
fn add a b
    a + b

print add 10 20

fn multiply x y
    x * y

print multiply 6 7
```

Pemanggilan fungsi dapat ditulis dengan gaya perintah `add 10 20` maupun gaya kurung `add(10, 20)`.

### Bab 7: Operasi Berkas (`read`, `write`)
Standard library menyediakan pembacaan dan penulisan berkas langsung secara native.

```eas
write "catatan.txt" "Belajar EasLang sangat mudah"
isi = read "catatan.txt"
print isi
```

### Bab 8: Objek & State Management (`new`, `set`, `get`)
Buat objek map/state baru dengan `new`, atur properti dengan `set`, dan ambil nilainya dengan `get`.

```eas
person = new
set person "role" "Engineer"
set person "level" "Senior"
print get person "role"
print get person "level"
```

### Bab 9: Jaringan & HTTP Client (`get`, `send`)
Permintaan HTTP GET dapat dilakukan langsung dengan keyword `get <url>`, dan pengiriman data dengan `send <url> <data>`.

```eas
response = get "httpbin.org"
print response
```

### Bab 10: Aplikasi Desktop Native Win32 (`app`, `window`, `run`)
Membangun antarmuka jendela desktop native secara langsung:

```eas
app "Aplikasi Saya"
window 800 600
print "Desktop window siap"
run 500
```
*Catatan: `run` menerima parameter timeout dalam milidetik atau berjalan terus hingga jendela ditutup pengguna.*

### Bab 11: Modularitas & Impor Modul (`use`)
Muat kode dari file `.eas` lain ke dalam scope program saat ini:

```eas
use "matematika.eas"
```

---

## 3. Kamus Keyword & Operator

### Kamus Keyword Lengkap

| Keyword | Kategori | Fungsi & Keterangan | Contoh Kode |
| :--- | :--- | :--- | :--- |
| `print` | Output | Menampilkan satu atau beberapa ekspresi ke konsol standar dengan spasi pemisah dan diakhiri baris baru | `print "Halo" 123` |
| `if` | Logika | Memulai blok percabangan kondisional berdasarkan nilai kebenaran (*truthiness*) | `if score > 75` |
| `else` | Logika | Blok alternatif jika kondisi `if` sebelumnya bernilai salah | `else` |
| `loop` | Iterasi | Mengulang eksekusi blok sebanyak $n$ kali secara terhitung | `loop 10` |
| `while` | Iterasi | Mengulang eksekusi blok selama ekspresi kondisional bernilai benar | `while x > 0` |
| `fn` | Fungsi | Mendeklarasikan fungsi baru dengan implicit return pada ekspresi terakhir | `fn calc a b` |
| `use` | Modul | Mengimpor dan mengeksekusi modul file `.eas` eksternal | `use "helper"` |
| `new` | Objek | Menginstansiasi map/objek baru di memori | `user = new` |
| `get` | Objek / HTTP | Mengambil properti objek (`get obj "key"`) atau melakukan HTTP GET (`get "url"`) | `get user "name"` |
| `set` | Objek | Menetapkan nilai properti pada objek/map (`set obj "key" val`) | `set user "age" 25` |
| `read` | File I/O | Membaca seluruh konten berkas teks dan mengembalikannya sebagai string | `text = read "data.txt"` |
| `write` | File I/O | Menulis teks ke berkas target secara native | `write "data.txt" "Konten"` |
| `send` | Jaringan | Mengirimkan data teks/payload ke URL tujuan (HTTP POST / Socket) | `send "url" payload` |
| `app` | Desktop GUI | Menetapkan judul untuk aplikasi jendela desktop | `app "Title Window"` |
| `window` | Desktop GUI | Mengatur lebar dan tinggi jendela GUI desktop native | `window 1024 768` |
| `run` | Desktop GUI | Memulai message pump dan lifecycle aplikasi desktop native | `run` atau `run 1000` |
| `end` | Struktur | Keyword penutup blok opsional bagi pengguna yang tidak ingin menggunakan indentasi murni | `end` |

### Kamus Operator & Simbol

| Operator | Fungsi | Penjelasan Singkat |
| :--- | :--- | :--- |
| `=` | Penetapan Nilai | Menyimpan hasil evaluasi ekspresi ke variabel |
| `+` | Penjumlahan / Konkatenasi | Menjumlahkan dua angka atau menggabungkan string |
| `-` | Pengurangan / Negasi | Mengurangi nilai atau memberikan nilai negatif |
| `*` | Perkalian | Mengalikan nilai numerik |
| `/` | Pembagian | Membagi dua angka secara presisi |
| `%` | Modulo | Menghitung sisa hasil bagi |
| `==`, `!=` | Kesetaraan | Memeriksa kesamaan atau perbedaan dua nilai |
| `<`, `>`, `<=`, `>=` | Relasional | Membandingkan besar-kecil nilai |
| `and`, `or`, `not` | Logika Boolean | Operator logika AND, OR, dan Negasi |
| `[]` | Indexer / Array Literal | Membuat list literal atau mengakses elemen berdasarkan indeks |
| `()` | Pengelompokan | Mengatur urutan prioritas evaluasi ekspresi |

---

## 4. Hasil Benchmark & Komparasi Kecepatan

Pengujian performa dilakukan secara langsung di lingkungan Windows 64-bit pada prosesor multi-core dengan membandingkan **EasLang Low-Machine Engine**, **EasLang Standalone Native Binary**, dan **Python 3.14**.

### Benchmark A: Suite Gabungan (2.000.000 Iterasi Loop + Fibonacci Rekursif $N=28$)

| Runtime / Engine | Waktu Eksekusi | Kecepatan Relatif vs Python |
| :--- | :--- | :--- |
| **EasLang Low-Machine Engine** | **88 ms** | **8.45x lebih cepat** |
| **Python 3.14** | **744 ms** | Baseline CPython standar |

### Benchmark B: Rekursif Fibonacci Mendalam ($N = 32$)

Menguji performa evaluasi rekursi bertingkat tinggi (4.356.617 pemanggilan fungsi) dengan arsitektur *direct register return*:

| Runtime / Engine | Waktu Eksekusi | Kecepatan Relatif vs Python |
| :--- | :--- | :--- |
| **EasLang Low-Machine Engine** | **150 ms** | **8.04x lebih cepat** |
| **Python 3.14** | **1.206 ms** | Baseline CPython standar |

### Benchmark C: Loop 10.000.000 Iterasi (Komputasi Intensif)

Menguji performa operasi perulangan dan aritmatika intensif berskala besar:

| Runtime / Engine | Waktu Eksekusi | Kecepatan Relatif vs Python |
| :--- | :--- | :--- |
| **EasLang Standalone Native Binary (`eas build`)** | **57 ms** | **37.8x lebih cepat** |
| **EasLang CLI Cached Engine (`eas script.eas`)** | **245 ms** | **8.8x lebih cepat** |
| **Python 3.14** | **2.157 ms** | Baseline CPython standar |

---

## 5. Panduan Build & Eksekusi

### Kompilasi Compiler (`eas.exe`)
Kompilasi source code compiler menggunakan g++ dengan standar C++20 dan optimasi native:

```bash
g++ -std=c++20 -O3 -march=native -flto src/*.cpp -Iinclude -lwininet -lgdi32 -luser32 -o eas.exe
```

### 1. Eksekusi Skrip Instan Tanpa Cache (Default CLI Mode)
Jalankan berkas skrip `.eas` secara langsung. Engine mengeksekusi secara instan di dalam memori tanpa meninggalkan berkas cache di penyimpanan disk (*zero disk cache*):

```bash
.\eas.exe script.eas
```

### 2. Kompilasi AOT ke Executable Mandiri (`build`)
Kompilasi skrip `.eas` langsung menjadi binary executable `.exe` native mandiri yang teroptimasi penuh tanpa dependensi runtime:

```bash
.\eas.exe build script.eas -o program.exe
.\program.exe
```

### 3. Mode Interaktif (Interactive REPL)
Jalankan `eas.exe` tanpa argumen untuk masuk ke interactive shell:

```bash
.\eas.exe
```
Ketik `exit` untuk keluar dari shell.

---

## 6. Lisensi

Proyek ini dirilis di bawah lisensi terbuka **MIT License**. Lihat berkas [`LICENSE`](LICENSE) untuk informasi lebih lanjut.

