# EasLang (.eas)

EasLang adalah bahasa pemrograman modern berperforma tinggi dengan filosofi sintaks ultra-bersih (ultra-clean syntax), manajemen memori deterministik tanpa Garbage Collector (Zero-Cost RAII), arsitektur dual-engine (Bytecode Virtual Machine & AOT Compiler), serta standard library bawaan yang lengkap (*batteries-included*).

Proyek ini dilisensikan di bawah **MIT License**.

---

## Daftar Isi
1. [Filosofi & Arsitektur](#1-filosofi--arsitektur)
2. [Tutorial & Panduan Instalasi](#2-tutorial--panduan-instalasi)
3. [Kursus Kilat EasLang (Crash Course)](#3-kursus-kilat-easlang-crash-course)
   - [Bab 13: Tutorial Kilat & Panduan Lengkap Seluruh Modul Bawaan](#bab-13-tutorial-kilat--panduan-lengkap-seluruh-modul-bawaan-standard-library-deep-dive)
4. [Kamus Keyword & Operator](#4-kamus-keyword--operator)
5. [Hasil Benchmark & Komparasi Kecepatan](#5-hasil-benchmark--komparasi-kecepatan)
6. [Panduan Eksekusi & Kompilasi AOT](#6-panduan-eksekusi--kompilasi-aot)
7. [Lisensi](#7-lisensi)

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

## 2. Tutorial & Panduan Instalasi

Seluruh berkas binary resmi EasLang otomatis dikompilasi oleh **GitHub Actions CI/CD** untuk multi-platform (**Linux x86_64**, **Linux ARM64**, **Android Termux ARM64**, dan **Windows x64**). Anda **tidak perlu repot mengompilasi manual**.

---

### 🐧 Tutorial Instalasi di Linux (Ubuntu / Debian / Arch / Fedora / WSL)

1. **Buka Terminal** Anda.
2. **Jalankan Installer 1-Baris**:
   ```bash
   curl -sSL https://raw.githubusercontent.com/sunandar3221/EasLang/main/install.sh | bash
   ```
   > 💡 **Apa yang dilakukan skrip ini?**
   > - Mendeteksi arsitektur CPU secara otomatis (`x86_64` atau `ARM64/aarch64`).
   > - Mengunduh binary prebuilt teroptimasi dari GitHub Releases.
   > - Memasang binary langsung ke `/usr/local/bin/eas` (atau `~/.local/bin/eas`).

3. **Verifikasi Instalasi**:
   Ketik perintah `eas` untuk masuk ke interactive REPL:
   ```bash
   eas
   ```
   Atau buat dan jalankan skrip pertama Anda:
   ```bash
   echo 'print "Halo dari EasLang di Linux!"' > halo.eas
   eas halo.eas
   ```

---

### 📱 Tutorial Instalasi di Android (Termux)

EasLang dapat berjalan secara native dan berkecepatan penuh di smartphone Android Anda menggunakan aplikasi **Termux**:

1. **Buka Aplikasi Termux** di Android Anda.
2. **Siapkan Paket Pendukung**:
   ```bash
   pkg update && pkg install curl -y
   ```
3. **Jalankan Installer 1-Baris**:
   ```bash
   curl -sSL https://raw.githubusercontent.com/sunandar3221/EasLang/main/install.sh | bash
   ```
   > 💡 **Kelebihan di Termux:**
   > - Mengunduh binary native Android Bionic (`eas-android-arm64`) yang dikompilasi langsung menggunakan Google Android NDK Clang.
   > - Otomatis terpasang ke `$PREFIX/bin/eas` sehingga tidak butuh akses root/sudo sama sekali.

4. **Verifikasi Instalasi**:
   Ketik perintah berikut di Termux:
   ```bash
   eas
   ```
5. **Coba Jalankan Skrip Pertama di Android**:
   ```bash
   echo 'nama = "Android Termux"' > coba.eas
   echo 'print "EasLang berjalan mulus di ${nama}"' >> coba.eas
   eas coba.eas
   ```

---

### 🪟 Tutorial Instalasi di Windows

Anda dapat memilih salah satu dari dua metode berikut:

#### Opsi 1: Unduh Cepat via PowerShell 1-Baris (Direkomendasikan)
Buka PowerShell (tekan `Win + X` lalu pilih Terminal/PowerShell) dan jalankan:
```powershell
Invoke-WebRequest -Uri "https://github.com/sunandar3221/EasLang/releases/latest/download/eas.exe" -OutFile "$HOME\AppData\Local\Microsoft\WindowsApps\eas.exe"
```
> ✨ Direktori `WindowsApps` sudah otomatis terdaftar di `PATH` Windows, sehingga Anda dapat langsung mengetik `eas` atau `eas.exe` dari folder/terminal mana saja tanpa perlu setting Environment Variables secara manual!

#### Opsi 2: Unduh Manual dari GitHub Releases
1. Kunjungi [Halaman Rilis GitHub EasLang](https://github.com/sunandar3221/EasLang/releases/latest).
2. Unduh berkas **`eas.exe`** (atau `eas-windows-x64.exe`).
3. Simpan berkas di folder pilihan Anda (misal `C:\EasLang\eas.exe`).
4. *(Opsional)* Tambahkan folder tersebut ke `PATH` di Environment Variables Windows.
5. Buka Command Prompt (CMD) atau PowerShell, lalu jalankan:
   ```cmd
   eas.exe
   ```

---

### 🛠️ Opsi: Kompilasi Mandiri dari Source Code (Khusus Developer)

Bagi pengembang yang ingin memodifikasi atau berkontribusi pada source code EasLang:

- **Linux / macOS / Termux**:
  ```bash
  git clone https://github.com/sunandar3221/EasLang.git
  cd EasLang
  make
  sudo make install   # (di Termux cukup: make install)
  ```

- **Windows (MinGW / GCC / Clang)**:
  ```bash
  git clone https://github.com/sunandar3221/EasLang.git
  cd EasLang
  g++ -std=c++20 -O3 -march=native -flto src/*.cpp -Iinclude -lwininet -lgdi32 -luser32 -o eas.exe
  ```

---

## 3. Kursus Kilat EasLang (Crash Course)

### Bab 1: Output, Variabel Dinamis, String Interpolation & Komentar
Di EasLang, cukup tulis nama variabel dan nilainya tanpa tipe data dan tanpa titik koma. Output ditampilkan menggunakan keyword `print`. Pemanggilan variabel di dalam string dapat ditulis langsung menggunakan **String Interpolation** (`${variabel}` atau `$variabel`), multi-argumen dengan spasi pemisah, maupun penggabungan string (`+`). Komentar dapat ditulis menggunakan `#` (gaya Python/Ruby) maupun `//` (gaya C/JavaScript):

```eas
# Ini adalah komentar satu baris gaya Python
// Ini adalah komentar satu baris gaya C/JS

name = "Budi"
age = 20  # Variabel dinamis otomatis terdefinisi

# 1. String Interpolation (${variabel} atau $variabel)
print "halo ${name}, umur kamu ${age}"
print "Nama: $name, Umur: $age tahun"

# 2. String Interpolation dengan Ekspresi Matematis / Fungsi
print "Tahun depan umur kamu: ${age + 1}"

# 3. Pemanggilan Standar & Multi-argumen
print name
print age
print "Halo nama saya " + name
print "Nama:", name, "Umur:", age
print("Halo Dunia")
```

> ⚡ **Performa Maksimal (Zero Runtime Overhead)**:
> String interpolation di EasLang diubah otomatis pada tahap parsing (*compile-time desugaring*) menjadi native concatenation. Ini membuat eksekusi string interpolation di Virtual Machine maupun biner native AOT berjalan dengan **kecepatan penuh tanpa latensi parsing di runtime**!


### Bab 2: Operasi Aritmatika & Ekspresi
EasLang mendukung operator standar `+`, `-`, `*`, `/`, `%` dengan prioritas matematis yang benar serta pengelompokan menggunakan tanda kurung `()`:

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

### Bab 3: List / Array Dinamis (`push`, `pop`, `len`)
List dideklarasikan dengan kurung siku `[]` dan diakses menggunakan indeks berbasis nol `[i]`. Tersedia fungsi bawaan `len(list)` untuk mengetahui panjang list, `push(list, item)` untuk menambah elemen, dan `pop(list)` untuk mengambil elemen terakhir:

```eas
buah = ["Apel", "Jeruk"]

# Menambah elemen baru ke list
push(buah, "Mangga")

# Mengetahui jumlah elemen dalam list
print len(buah)          # Output: 3

# Mengakses elemen berdasarkan indeks
print buah[0]            # Output: Apel
print buah[2]            # Output: Mangga

# Mengambil dan menghapus elemen terakhir
terakhir = pop(buah)
print "Dihapus: " + terakhir  # Output: Mangga
print len(buah)          # Output: 2
```

### Bab 4: Percabangan (`if` - `elseif` / `elif` - `else`), If Expression, & Pencocokan Teks
Percabangan menggunakan kata kunci `if`, `elseif` (atau `elif`), dan `else` berbasis indentasi bersih tanpa kurung kurawal atau titik dua. Keyword `end` dapat digunakan secara opsional.

#### 1. Percabangan Bersarang & Multikondisi (`elseif` / `elif`)
Anda dapat menggunakan `elseif` maupun `elif` untuk mengecek banyak kondisi secara berurutan:
```eas
nilai = 85

if nilai >= 90
    print "Grade A"
elseif nilai >= 80
    print "Grade B"
elif nilai >= 70
    print "Grade C"
else
    print "Grade D"
```

#### 2. If Expression (Menugaskan Nilai Percabangan ke Variabel)
Hasil evaluasi `if` dapat ditugaskan langsung ke sebuah variabel sebagai ekspresi yang sangat bersih (*clean ternary*):
```eas
nilai = 85

status = if nilai >= 75
    "Selamat Anda Lulus"
else
    "Silakan Mengulang Ujian"

print status  # Output: Selamat Anda Lulus
```

#### 3. Pencocokan Teks: Case Sensitive & Incase Sensitive
EasLang menyediakan fungsi bawaan untuk membedakan maupun menyamakan perbandingan string antara huruf kapital dan non-kapital:
- **`case_sensitive(a, b)`**: Membedakan huruf kapital dan non-kapital secara ketat (menghasilkan `true` jika kedua teks persis sama, misal `"Agus"` dengan `"Agus"` adalah `true`, tapi `"Agus"` dengan `"agus"` adalah `false`).
- **`incase_sensitive(a, b)`**: Menyamakan huruf kapital dan non-kapital (*case-insensitive*, menghasilkan `true` meskipun huruf besar/kecil berbeda, misal `"Agus"` dengan `"agus"` adalah `true`).
- **`lower(teks)`**: Mengubah teks menjadi huruf kecil (*lowercase*).
- **`upper(teks)`**: Mengubah teks menjadi huruf besar kapital (*uppercase*).

```eas
nama = "Agus"

# Menyamakan huruf kapital dan non-kapital (Incase Sensitive)
if incase_sensitive(nama, "agus")
    print "Nama cocok (mengabaikan huruf besar/kecil)!"

# Membedakan huruf kapital dan non-kapital (Case Sensitive)
if case_sensitive(nama, "agus")
    print "Sama persis"
else
    print "Huruf besar/kecil berbeda: 'Agus' != 'agus'"

# Mengubah bentuk huruf
print lower("Halo Dunia")  # Output: halo dunia
print upper("halo dunia")  # Output: HALO DUNIA
```

### Bab 5: Perulangan (`loop` & `while`), `break`, dan `continue`
- Gunakan `loop <jumlah>` untuk mengulang blok sebanyak $n$ kali secara terhitung.
- Gunakan `while <kondisi>` untuk perulangan berbasis evaluasi kondisi.
- Gunakan `break` untuk keluar dari loop seketika.
- Gunakan `continue` untuk melompati sisa iterasi saat ini dan lanjut ke iterasi berikutnya.

```eas
# Perulangan loop terhitung
loop 3
    print "Pengulangan loop"

# Perulangan while dengan break dan continue
i = 0
while true
    i = i + 1
    if i == 3
        continue  # Lewati angka 3
    if i > 5
        break     # Keluar saat i melebihi 5
    print "Angka: ${i}"
```

#### Loop Assignment & `silent_print`
Hasil evaluasi `loop` dapat ditugaskan langsung ke variabel untuk mengumpulkan seluruh outputnya. Gunakan `silent_print` agar memformat dan menampung baris teks tanpa membanjiri layar terminal (karena operasi berkas memerlukan `io`, pastikan memuat `use io`):

```eas
use io

halo = loop 3000
    silent_print "kamu manusia apa apa"

write "hai.txt" halo
```

### Bab 6: Fungsi, Explicit & Implicit Return, serta Kata Kunci Alternatif (`fn`, `def`, `func`)
Fungsi dapat dideklarasikan dengan `fn`, `def`, `func`, maupun `function`, diikuti nama fungsi dan parameter yang dipisahkan spasi.
- **Implicit & Explicit Return**: Baris terakhir otomatis menjadi nilai balik fungsi, ATAU Anda dapat menggunakan kata kunci `return` untuk mengembalikan nilai secara eksplisit / keluar lebih awal (*early exit*).
- **Fleksibilitas Pemanggilan**: Fungsi dapat dipanggil dengan gaya perintah `add 10 20` maupun gaya kurung `add(10, 20)`.

```eas
# Menggunakan kata kunci 'fn' dengan implicit return
fn add a b
    a + b

print add 10 20
print add(10, 20)

# Menggunakan 'def' / 'func' dengan 'return' eksplisit
def cekStatus umur
    if umur < 0
        return "Tidak valid"
    elseif umur < 18
        return "Minor"
    return "Adult"

print cekStatus(20) # Output: Adult
```

#### Memasukkan Fungsi ke Variabel (First-Class Functions)
Fungsi di EasLang dapat disimpan ke dalam variabel dan dideklarasikan secara dinamis:

```eas
operasi = add
print operasi 15 25

kali = fn x y
    x * y

print kali 4 5
```

### Bab 7: Pustaka I/O & Input Interaktif (`use io` / `import io`)
Untuk menggunakan fitur input-output berkas dan interaksi pengguna, Anda **wajib** memuat pustaka `io` terlebih dahulu menggunakan `use io` atau `import io` (seperti halnya di Python):

```eas
use io

nama = io.input("Siapa nama kamu? ")
umur = input "Berapa umur kamu? "
hobi = io.ask("Apa hobi kamu? ")

io.print "Halo ${nama}, umur ${umur}, hobi ${hobi}"

io.write "catatan.txt", "Belajar EasLang sangat menyenangkan"
isi = io.read "catatan.txt"
print isi
```

### Bab 8: Objek, State Management & Dot Notation (`new`, `set`, `get`)
Buat objek map/state baru dengan `new`, atur properti dengan `set`, dan ambil nilainya dengan `get` atau notasi titik (`.`):

```eas
person = new
set person "role" "Engineer"
set person "level" "Senior"

print get person "role"
print person.level

set person "sapa" (fn nama
    "Halo " + nama)
```

### Bab 9: Jaringan & HTTP Client (`use net`, `net.get`, `net.send`)
Permintaan HTTP GET dapat dilakukan langsung dengan memuat library `net`:

```eas
use net

response = net.get "httpbin.org"
print response
```

### Bab 10: Aplikasi Desktop Native Win32 (`use gui`, `app`, `window`, `run`)
Membangun antarmuka jendela desktop native secara langsung:

```eas
use gui

app "Aplikasi Saya"
window 800 600
print "Desktop window siap"
run 500
```
*Catatan: `run` menerima parameter timeout dalam milidetik atau berjalan terus hingga jendela ditutup pengguna.*

### Bab 11: Modularitas, Pustaka Bawaan & Cara Membuat Library Sendiri (`use` / `import`)

EasLang mendukung sistem modularitas modern yang fleksibel menggunakan kata kunci `use` atau `import`.

#### A. Menggunakan Library Bawaan (Built-in Standard Libraries)
EasLang menyertakan pustaka standar bawaan berkecepatan tinggi:
- **`io`**: Input/output berkas (`read`, `write`, `io.read`, `io.write`) dan input pengguna interaktif (`input`, `io.input`, `io.ask`). Wajib dimuat sebelum digunakan!
- **`math`**: Operasi matematika presisi tinggi (`math.sqrt`, `math.abs`, `math.pow`, `math.pi`, dll.).
- **`time`**: Operasi waktu dan jeda eksekusi (`time.now`, `time.sleep`).
- **`net`** / **`http`**: Komunikasi jaringan dan request HTTP (`net.get`, `net.send`).
- **`gui`**: Antarmuka grafis desktop native Win32 (`gui.app`, `gui.window`, `gui.run`).

Contoh pemanggilan pustaka standar:
```eas
import io
import math
import time

angka = 16
akar = math.sqrt(angka)
io.print "Akar dari 16 adalah: " + str(akar)

time.sleep 1000
io.print "Selesai jeda 1 detik"
```

---

#### B. Tutorial: Cara Membuat Library / Modul Sendiri
Membuat library di EasLang sangat sederhana. Anda cukup membuat berkas `.eas` baru dan mendefinisikan fungsi, variabel, atau objek yang ingin digunakan kembali oleh program lain.

##### Langkah 1: Buat Berkas Library (Contoh: `kalkulator.eas`)
Simpan file ini dengan nama `kalkulator.eas`:
```eas
phi = 3.14159

fn tambah a b
    a + b

fn kurang a b
    a - b

fn kali a b
    a * b

fn bagi a b
    a / b

fn luas_lingkaran r
    phi * r * r
```

> 💡 **Tips Pengorganisasian dengan Objek (Namespacing):**
> Anda juga dapat membungkus fungsi-fungsi library ke dalam suatu objek:
> ```eas
> sapaFn = fn nama
>     "Halo " + nama
> 
> helper = new
> set helper "sapa" sapaFn
> ```

---

#### C. Tutorial: Cara Mengimpor & Menggunakan Library
Gunakan perintah `import` atau `use` pada skrip utama Anda.

##### Langkah 2: Buat Skrip Utama (Contoh: `main.eas`)
Simpan di direktori yang sama dengan `kalkulator.eas`:
```eas
use io
import "kalkulator"

hasilTambah = tambah 15 25
io.print "15 + 25 = ${hasilTambah}"

hasilKali = kali(6, 7)
io.print "6 * 7 = ${hasilKali}"

r = 10
luas = luas_lingkaran r
io.print "Luas lingkaran (r=10): ${luas}"
```

##### Berbagai Format Penulisan Impor yang Didukung:
EasLang memberikan fleksibilitas tinggi dalam cara pemanggilan modul:
1. **Dengan Tanda Kutip**:
   - `import "kalkulator"` atau `use "kalkulator"`
   - `import "kalkulator.eas"` atau `use "kalkulator.eas"`
2. **Tanpa Tanda Kutip (Gaya Python / Ruby)**:
   - `import kalkulator` atau `use kalkulator`
3. **Fleksibilitas Pemanggilan Fungsi**:
   - Fungsi dari library yang diimpor dapat dipanggil dengan spasi tanpa tanda kurung (`tambah 10 20`) ataupun dengan tanda kurung (`tambah(10, 20)`).

---

### Bab 12: Fungsi Bawaan Global & Konversi Tipe Data

EasLang menyediakan fungsi bawaan global tingkat sistem yang dapat digunakan langsung tanpa perlu memuat pustaka tambahan:

#### 1. Konversi Tipe Data
- **`str(val)`**: Mengonversi nilai apa pun (angka, boolean, list, objek) menjadi bentuk teks string.
- **`int(val)`**: Mengonversi nilai angka/string menjadi integer 64-bit.
- **`float(val)`**: Mengonversi nilai menjadi floating-point presisi ganda (64-bit float).

```eas
teks = "123"
angka = int(teks)
desimal = float("3.1415")
hasilGabung = "Nilai: ${angka}"  # atau "Nilai: " + str(angka)
print hasilGabung
```

#### 2. Inspeksi Ukuran & Koleksi Data
- **`len(target)`**: Menghitung panjang karakter string, jumlah elemen list, atau jumlah kunci pada objek map.
- **`push(list, val)`**: Menambahkan elemen ke urutan terakhir list.
- **`pop(list)`**: Mengambil dan menghapus elemen terakhir list.

```eas
# Mengetahui panjang string & list
print len("EasLang")          # Output: 7

daftar = [10, 20]
push(daftar, 30)
print len(daftar)             # Output: 3
print pop(daftar)             # Output: 30
```

#### 3. Manipulasi & Pencocokan String
- **`lower(str)`**: Mengubah teks menjadi huruf kecil (*lowercase*).
- **`upper(str)`**: Mengubah teks menjadi huruf kapital (*uppercase*).
- **`case_sensitive(a, b)`**: Memeriksa apakah dua teks sama persis dengan membedakan huruf kapital.
- **`incase_sensitive(a, b)`**: Memeriksa apakah dua teks sama dengan menyamakan huruf kapital/non-kapital (*case-insensitive*).

```eas
kalimat = "Belajar EasLang"
print lower(kalimat)          # Output: belajar easlang
print upper(kalimat)          # Output: BELAJAR EASLANG

user1 = "admin"
user2 = "ADMIN"
print incase_sensitive(user1, user2)  # Output: true
print case_sensitive(user1, user2)    # Output: false
```

---

### Bab 13: Tutorial Kilat & Panduan Lengkap Seluruh Modul Bawaan (Standard Library Deep Dive)

EasLang menganut filosofi *batteries-included*, di mana pustaka-pustaka esensial untuk kebutuhan komputasi matematika, I/O berkas & input keyboard, pengukuran waktu, jaringan HTTP, dan antarmuka jendela desktop native telah terintegrasi langsung di dalam engine tanpa perlu mengunduh package manager eksternal. Anda cukup mengaktifkannya menggunakan keyword `use <nama_modul>` atau `import <nama_modul>`.

---

#### 1. Modul `math` (Matematika, Trigonometri & Random Number Generator)
Modul `math` menyediakan fungsi-fungsi perhitungan matematika berkecepatan tinggi, operasi trigonometri, konstanta fundamental, serta generator angka acak presisi tinggi (PRNG berbasis Mersenne Twister 64-bit `std::mt19937_64`).

##### A. Kamus Fungsi Modul `math`
| Fungsi / Properti | Penjelasan | Contoh Penggunaan |
| :--- | :--- | :--- |
| `math.random(min, max)` | Menghasilkan angka integer acak antara `min` dan `max` (inklusif). Jika tanpa argumen, menghasilkan float acak $0.0 \le x < 1.0$. | `dadu = math.random(1, 6)` |
| `math.random_seed(seed)` *(atau `math.seed`)* | Mengatur *seed* generator acak dengan nilai integer tertentu untuk menghasilkan urutan acak yang **deterministik / dapat diulang**. Sangat berguna untuk pengujian unit, savegame, atau procedural generation. | `math.random_seed(42)` |
| `math.random_seed()` | Jika dipanggil tanpa argumen, fungsi ini akan me-reset seed menggunakan **entropi hardware murni** (`std::random_device`) yang digabung dengan clock prosesor beresolusi tinggi, sehingga angka acak berikutnya dijamin tak terduga (*unpredictable*). | `math.random_seed()` |
| `math.sqrt(x)` | Menghitung akar kuadrat dari $x$. | `math.sqrt(81)` $\rightarrow$ `9.0` |
| `math.pow(base, exp)` | Menghitung perpangkatan $base^{exp}$. | `math.pow(2, 8)` $\rightarrow$ `256.0` |
| `math.abs(x)` | Mengembalikan nilai absolut positif dari $x$. | `math.abs(-25)` $\rightarrow$ `25` |
| `math.floor(x)` | Membulatkan angka pecahan ke bawah ke integer terdekat. | `math.floor(4.9)` $\rightarrow$ `4` |
| `math.ceil(x)` | Membulatkan angka pecahan ke atas ke integer terdekat. | `math.ceil(4.1)` $\rightarrow$ `5` |
| `math.round(x)` | Membulatkan angka desimal ke integer terdekat (standar matematika). | `math.round(4.6)` $\rightarrow$ `5` |
| `math.min(a, b)` | Mengembalikan nilai terkecil di antara dua angka. | `math.min(10, 5)` $\rightarrow$ `5` |
| `math.max(a, b)` | Mengembalikan nilai terbesar di antara dua angka. | `math.max(10, 5)` $\rightarrow$ `10` |
| `math.sin(rad)` | Menghitung nilai sinus (sudut dalam radian). | `math.sin(0)` $\rightarrow$ `0.0` |
| `math.cos(rad)` | Menghitung nilai kosinus (sudut dalam radian). | `math.cos(0)` $\rightarrow$ `1.0` |
| `math.tan(rad)` | Menghitung nilai tangen (sudut dalam radian). | `math.tan(0)` $\rightarrow$ `0.0` |
| `math.pi` | Konstanta $\pi$ bernilai $3.141592653589793...$ | `keliling = 2 * math.pi * r` |
| `math.e` | Konstanta bilangan Euler $e$ bernilai $2.718281828459...$ | `math.e` |

##### B. Contoh Praktis: Mengontrol Random Seed & Game Tebak Angka
Secara default, saat Anda menjalankan EasLang, generator angka acak **otomatis menggunakan entropi perangkat keras** sehingga setiap kali program dijalankan, angka tebakan akan selalu berbeda dan tidak bisa diprediksi. Namun jika Anda membutuhkan hasil yang konsisten (misalnya saat testing), Anda dapat menetapkan seed secara manual:

```eas
use io
use math

# --- 1. Contoh Seed Deterministik (Hasil selalu sama setiap kali dijalankan) ---
math.random_seed(12345)
print "Acak 1: ${math.random(1, 100)}"
print "Acak 2: ${math.random(1, 100)}"

# --- 2. Reset ke Entropi Hardware Murni (Tidak bisa ditebak) ---
math.random_seed()

# --- 3. Aplikasi: Game Tebak Angka Interaktif ---
angkaRahasia = math.random(1, 100)
selesai = false
percobaan = 0

print "🎯 GAME TEBAK ANGKA RESMI EASLANG"
print "Komputer telah memilih angka acak antara 1 s.d. 100!"

while selesai == false
    percobaan = percobaan + 1
    tebakan = int(input("Masukkan tebakan ke-${percobaan}: "))

    if tebakan < angkaRahasia
        print "📉 Terlalu KECIL! Coba angka yang lebih besar."
    elseif tebakan > angkaRahasia
        print "📈 Terlalu BESAR! Coba angka yang lebih kecil."
    else
        print "🎉 SELAMAT! Tebakanmu BENAR!"
        print "Angkanya adalah: ${angkaRahasia}"
        print "Kamu berhasil menebak dalam ${percobaan} kali percobaan."
        selesai = true
```

---

#### 2. Modul `io` (Input Keyboard Interaktif & File I/O Native)
Modul `io` mengelola interaksi konsol serta pembacaan dan penulisan berkas pada filesystem secara cepat dan aman.

##### A. Kamus Fungsi Modul `io`
| Fungsi | Penjelasan | Contoh Penggunaan |
| :--- | :--- | :--- |
| `input(prompt)` *(atau `io.input`)* | Menampilkan teks pertanyaan ke layar konsol dan membaca satu baris input dari pengguna | `nama = input("Nama Anda: ")` |
| `ask(prompt)` *(atau `io.ask`)* | Alternatif penulisan semantik dari fungsi `input` | `umur = ask("Umur: ")` |
| `write(filepath, teks)` *(atau `io.write`)* | Menulis seluruh teks string ke berkas secara native (otomatis menimpa/membuat baru) | `write "pesan.txt" "Halo Dunia"` |
| `read(filepath)` *(atau `io.read`)* | Membaca seluruh isi berkas teks dari disk ke dalam memori | `konten = read "pesan.txt"` |
| `io.print(...)` | Menampilkan teks output konsol yang terisolasi di dalam namespace `io` | `io.print "Data berhasil disimpan"` |

##### B. Contoh Praktis: Sistem Catatan & Penyimpanan Skor (High-Score Logger)
```eas
use io

print "=== SISTEM PENCATAT SKOR GAME ==="
pemain = input("Masukkan Nama Pemain: ")
skor = input("Masukkan Skor Tertinggi: ")

# Menulis data ke berkas teks di disk
catatan = "Pemain: ${pemain} | Skor: ${skor}\n"
write "leaderboard.txt" catatan
print "✅ Data pemain berhasil disimpan ke leaderboard.txt"

# Membaca kembali berkas dari disk
print "\n📄 Isi Berkas leaderboard.txt Saat Ini:"
isi = read "leaderboard.txt"
print isi
```

---

#### 3. Modul `time` (Waktu Presisi Tinggi & Delay Eksekusi)
Modul `time` berguna untuk mengukur performa baris kode (profiling / stopwatch), jeda proses (*sleeping/delay*), atau penanda waktu UNIX.

##### A. Kamus Fungsi Modul `time`
| Fungsi | Penjelasan | Contoh Penggunaan |
| :--- | :--- | :--- |
| `time.now()` | Mengembalikan waktu epoch saat ini dalam satuan **milidetik** (integer 64-bit) | `mulai = time.now()` |
| `time.sleep(ms)` | Menghentikan sementara (*pause*) eksekusi program selama $ms$ milidetik | `time.sleep 1000` *(jeda 1 detik)* |

##### B. Contoh Praktis: Pengukur Durasi Eksekusi & Hitung Mundur
```eas
use io
use time

print "⏳ Memulai Hitung Mundur..."
loop 3
    print "Detik..."
    time.sleep 1000  # Jeda 1.000 milidetik (1 detik)

# Mengukur kecepatan komputasi algoritma (Benchmark stopwatch)
waktuAwal = time.now()

total = 0
i = 0
while i < 1000000
    i = i + 1
    total = total + i

waktuAkhir = time.now()
durasi = waktuAkhir - waktuAwal

print "Hasil penjumlahan 1 s.d 1.000.000: ${total}"
print "⚡ Waktu komputasi yang dibutuhkan: ${durasi} ms"
```

---

#### 4. Modul `net` / `http` (HTTP Client & Komunikasi Jaringan)
Modul `net` (bisa juga diakses dengan alias `http`) memungkinkan program EasLang melakukan permintaan jaringan berbasis protokol HTTP GET dan POST secara langsung.

##### A. Kamus Fungsi Modul `net` / `http`
| Fungsi | Penjelasan | Contoh Penggunaan |
| :--- | :--- | :--- |
| `net.get(url)` *(atau `http.get`)* | Mengirimkan permintaan HTTP GET ke alamat URL yang ditentukan dan mengembalikan respons teks dari server | `resp = net.get "httpbin.org/get"` |
| `net.send(url, payload)` *(atau `http.send`)* | Mengirimkan data teks/payload via HTTP POST ke endpoint tujuan | `res = net.send "api.site/data" "key=val"` |

##### B. Contoh Praktis: Mengambil Data dari REST API
```eas
use io
use net

print "🌐 Mengambil data dari server API..."
data = net.get "httpbin.org/uuid"
print "Respons dari Server:"
print data
```

---

#### 5. Modul `gui` (Aplikasi Antarmuka Jendela Desktop Native Win32)
Modul `gui` menyediakan API minimalis berkinerja tinggi untuk memunculkan antarmuka jendela desktop native pada sistem operasi Windows tanpa memerlukan instalasi library UI pihak ketiga yang berat.

##### A. Kamus Fungsi Modul `gui`
| Fungsi | Penjelasan | Contoh Penggunaan |
| :--- | :--- | :--- |
| `gui.app(judul)` *(atau `app judul`)* | Menginisialisasi aplikasi GUI dan menentukan judul bilah jendela (*window title*) | `gui.app "Kalkulator EasLang"` |
| `gui.window(lebar, tinggi)` *(atau `window w h`)* | Mengatur dimensi ukuran jendela dalam piksel | `gui.window 800 600` |
| `gui.run(timeout_ms)` *(atau `run`)* | Menjalankan message loop native. Jika diberi parameter milidetik, jendela akan otomatis tertutup setelah waktu habis | `gui.run 3000` *(tampil selama 3 detik)* |

##### B. Contoh Praktis: Memunculkan Jendela GUI Desktop
```eas
use io
use gui

print "Mempersiapkan jendela native..."
gui.app "Dashboard Monitoring EasLang"
gui.window 1024 768
print "Jendela berhasil dibuat di layar pengguna!"

# Menjalankan lifecycle window selama 5 detik (atau hingga ditutup pengguna)
gui.run 5000
```

---

#### 6. Modul `str` (Manipulasi Teks Tingkat Lanjut)
Meskipun fungsi manipulasi teks umum tersedia secara global, modul `str` menyediakan namespace terstruktur untuk pengelolaan dan standardisasi string dalam proyek besar.

##### A. Kamus Fungsi Modul `str`
| Fungsi | Penjelasan | Contoh Penggunaan |
| :--- | :--- | :--- |
| `str.lower(teks)` | Mengubah seluruh abjad dalam teks menjadi huruf kecil | `str.lower("EasLang")` $\rightarrow$ `"easlang"` |
| `str.upper(teks)` | Mengubah seluruh abjad dalam teks menjadi huruf kapital | `str.upper("EasLang")` $\rightarrow$ `"EASLANG"` |
| `str.case_sensitive(a, b)` | Membandingkan kecocokan dua teks secara ketat dengan membedakan huruf besar/kecil | `str.case_sensitive("A", "a")` $\rightarrow$ `false` |
| `str.incase_sensitive(a, b)` | Membandingkan kecocokan dua teks tanpa mempedulikan huruf besar/kecil (*case-insensitive*) | `str.incase_sensitive("Admin", "admin")` $\rightarrow$ `true` |

##### B. Contoh Praktis: Validasi Autentikasi Form
```eas
use io
use str

usernameDB = "admin_super"
passwordDB = "Rahasia123!"

inputUser = input("Username: ")
inputPass = input("Password: ")

# Username bersifat case-insensitive (huruf besar/kecil sama saja)
userCocok = str.incase_sensitive(inputUser, usernameDB)

# Password bersifat case-sensitive (huruf besar/kecil wajib persis sama)
passCocok = str.case_sensitive(inputPass, passwordDB)

if userCocok and passCocok
    print "🔓 Akses DITERIMA! Selamat datang, Administrator."
elseif userCocok and not passCocok
    print "❌ Akses DITOLAK: Password salah (perhatikan huruf besar/kecil)!"
else
    print "❌ Akses DITOLAK: Akun pengguna tidak ditemukan!"
```

---

## 4. Kamus Keyword, Fungsi Bawaan & Operator

### 4.1 Kamus Keyword Utama

| Keyword | Kategori | Fungsi & Keterangan | Contoh Kode |
| :--- | :--- | :--- | :--- |
| `print` | Output | Menampilkan satu atau beberapa nilai ke konsol standar dengan spasi pemisah dan diakhiri baris baru | `print "Halo" 123` |
| `silent_print` | Output | Memformat teks seperti `print` tanpa mencetak ke konsol terminal (menampung output untuk loop assignment) | `silent_print "Data"` |
| `if` | Logika | Memulai blok percabangan kondisional berbasis indentasi bersih | `if score > 75` |
| `elseif` / `elif` | Logika | Percabangan alternatif multikondisi jika kondisi sebelumnya tidak terpenuhi | `elseif score >= 60` |
| `else` | Logika | Blok alternatif jika seluruh kondisi `if` / `elseif` sebelumnya salah | `else` |
| `loop` | Iterasi | Mengulang eksekusi blok sebanyak $n$ kali secara terhitung | `loop 10` |
| `while` | Iterasi | Mengulang eksekusi blok selama ekspresi kondisional bernilai benar | `while x > 0` |
| `break` | Kontrol Loop | Menghentikan eksekusi perulangan (`while` atau `loop`) dan keluar seketika | `break` |
| `continue` | Kontrol Loop | Melompati sisa baris iterasi saat ini dan langsung ke iterasi loop berikutnya | `continue` |
| `return` | Fungsi | Mengembalikan nilai dari fungsi secara eksplisit / keluar lebih awal | `return hasil` |
| `fn` / `def` / `func` | Fungsi | Mendeklarasikan fungsi baru (mendukung implicit dan explicit return) | `def calc a b` atau `fn x` |
| `use` / `import` | Modul | Mengimpor berkas modul `.eas` eksternal atau pustaka bawaan (didukung juga `include` / `require`) | `use io` atau `import math` |
| `new` | Objek | Menginstansiasi objek map/state baru di memori | `user = new` |
| `get` | Objek / HTTP | Mengambil properti objek (`get obj "key"`) atau melakukan HTTP GET (`get "url"`) | `get user "name"` |
| `set` | Objek | Menetapkan nilai properti pada objek/map (`set obj "key" val`) | `set user "age" 25` |
| `read` | File I/O | Membaca seluruh isi berkas teks (membutuhkan `use io` / `import io`) | `text = read "data.txt"` |
| `write` | File I/O | Menulis teks ke berkas target secara native (membutuhkan `use io` / `import io`) | `write "data.txt" "Konten"` |
| `send` | Jaringan | Mengirimkan data teks/payload ke URL tujuan (HTTP POST / Socket) | `send "url" payload` |
| `app` | Desktop GUI | Menetapkan judul untuk aplikasi jendela desktop (`use gui`) | `app "Title Window"` |
| `window` | Desktop GUI | Mengatur lebar dan tinggi jendela GUI desktop native (`use gui`) | `window 1024 768` |
| `run` | Desktop GUI | Memulai message pump dan lifecycle aplikasi desktop native (`use gui`) | `run` atau `run 1000` |
| `end` | Struktur | Keyword penutup blok opsional bagi pengguna yang ingin penutup eksplisit | `end` |
| `true` / `True` | Nilai | Literal boolean benar | `isAktif = true` |
| `false` / `False` | Nilai | Literal boolean salah | `isAktif = false` |
| `nil` / `null` / `None` | Nilai | Literal nilai kosong / ketiadaan nilai | `data = nil` |

### 4.2 Kamus Fungsi Bawaan Global (Built-in Functions)

Fungsi-fungsi ini dapat dipanggil langsung dari mana saja tanpa perlu import/use:

| Fungsi | Parameter | Nilai Balik | Keterangan & Contoh |
| :--- | :--- | :--- | :--- |
| `len(x)` | String, List, Objek | `int` | Menghitung panjang teks, jumlah item list, atau jumlah properti objek (`len("Halo")` $\rightarrow$ `4`) |
| `str(x)` | Nilai apa saja | `string` | Mengonversi nilai apa pun menjadi string (`str(123)` $\rightarrow$ `"123"`) |
| `int(x)` | Angka, String | `int` | Mengonversi nilai menjadi integer 64-bit (`int("50")` $\rightarrow$ `50`) |
| `float(x)` | Angka, String | `float` | Mengonversi nilai menjadi angka pecahan floating-point (`float("3.14")` $\rightarrow$ `3.14`) |
| `push(list, val)` | List, Nilai baru | `val` | Menambahkan elemen baru ke akhir list (`push(buah, "Apel")`) |
| `pop(list)` | List | Nilai terakhir | Menghapus dan mengembalikan elemen terakhir list (`terakhir = pop(buah)`) |
| `case_sensitive(a, b)` | Dua teks string | `bool` | Membandingkan dua string dengan membedakan huruf kapital (*case-sensitive*) |
| `incase_sensitive(a, b)` | Dua teks string | `bool` | Membandingkan dua string dengan menyamakan huruf kapital (*case-insensitive*) |
| `lower(str)` | String | `string` | Mengubah seluruh karakter teks menjadi huruf kecil (*lowercase*) |
| `upper(str)` | String | `string` | Mengubah seluruh karakter teks menjadi huruf besar kapital (*uppercase*) |
| `input(prompt)` | String prompt opsional | `string` | Membaca input teks interaktif dari terminal pengguna (membutuhkan `use io`) |
| `ask(prompt)` | String prompt opsional | `string` | Sinonim dari `input` untuk meminta respon teks pengguna (membutuhkan `use io`) |

### 4.3 Kamus Pustaka Standar Bawaan (Standard Modules)

| Pustaka | Cara Memuat | Fitur & Fungsi Utama | Keterangan |
| :--- | :--- | :--- | :--- |
| **`io`** | `use io` atau `import io` | `io.input`, `io.ask`, `io.print`, `io.read`, `io.write` | Wajib dimuat untuk operasi berkas dan input keyboard pengguna |
| **`math`** | `use math` atau `import math` | `math.sqrt`, `math.pow`, `math.abs`, `math.floor`, `math.ceil`, `math.round`, `math.min`, `math.max`, `math.random`, `math.random_seed`, `math.sin`, `math.cos`, `math.tan`, `math.pi`, `math.e` | Perhitungan matematis, trigonometri, dan generator angka acak. Mendukung penetapan seed manual (`math.random_seed(seed)`) untuk hasil deterministik atau reset ke entropi hardware murni (`math.random_seed()`). |
| **`time`** | `use time` atau `import time` | `time.now`, `time.sleep` | Pengukuran waktu (milidetik) dan jeda eksekusi program |
| **`net`** / **`http`** | `use net` atau `import net` | `net.get`, `net.send`, `http.get`, `http.send` | Komunikasi jaringan dan request HTTP |
| **`gui`** | `use gui` atau `import gui` | `gui.app`, `gui.window`, `gui.run` | Aplikasi GUI jendela desktop native Win32 |
| **`str`** | `use str` atau `import str` | `str.lower`, `str.upper`, `str.case_sensitive`, `str.incase_sensitive` | Modul pembantu pemrosesan string dan manipulasi teks |

### 4.4 Kamus Operator, Simbol & Komentar

| Simbol / Operator | Kategori | Penjelasan Singkat | Contoh Kode |
| :--- | :--- | :--- | :--- |
| `#` | Komentar | Menandai baris komentar satu baris (gaya Python/Ruby) | `# Ini komentar` |
| `//` | Komentar | Menandai baris komentar satu baris (gaya C/JavaScript) | `// Ini komentar` |
| `=` | Penetapan Nilai | Menyimpan hasil evaluasi ekspresi ke variabel | `x = 10` |
| `.` | Member Access | Mengakses properti objek atau fungsi pustaka | `io.write`, `person.role` |
| `+` | Penjumlahan / Concat | Menjumlahkan dua angka atau menggabungkan string | `10 + 20`, `"A" + "B"` |
| `-` | Pengurangan / Negasi | Mengurangi nilai atau memberikan tanda negatif | `50 - 20`, `-x` |
| `*` | Perkalian | Mengalikan nilai numerik | `6 * 7` |
| `/` | Pembagian | Membagi dua angka secara presisi | `100 / 4` |
| `%` | Modulo | Menghitung sisa hasil bagi | `10 % 3` |
| `==`, `!=` | Kesetaraan | Memeriksa kesamaan atau perbedaan dua nilai | `a == b`, `x != y` |
| `<`, `>`, `<=`, `>=` | Relasional | Membandingkan besar-kecil nilai | `score >= 75` |
| `and`, `or`, `not`, `&&`, `\|\|`, `!` | Logika Boolean | Operator logika AND, OR, dan NOT (kata atau simbol) | `if a and not b`, `x && !y` |
| `[]` | Indexer / Array Literal | Membuat list literal atau mengakses elemen via indeks | `arr = [1, 2]`, `arr[0]` |
| `()` | Prioritas / Panggilan | Mengatur prioritas ekspresi atau memanggil fungsi | `(a + b) * c`, `add(1, 2)` |

---

## 5. Hasil Benchmark & Komparasi Kecepatan

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

## 6. Panduan Eksekusi & Kompilasi AOT

### 1. Eksekusi Skrip Instan Tanpa Cache (Default CLI Mode)
Jalankan berkas skrip `.eas` secara langsung. Engine mengeksekusi secara instan di dalam memori tanpa meninggalkan berkas cache di penyimpanan disk (*zero disk cache*):

```bash
# Di Linux & Android (Termux):
eas script.eas

# Di Windows:
.\eas.exe script.eas
```

### 2. Kompilasi AOT ke Executable Mandiri (`build`)
Kompilasi skrip `.eas` langsung menjadi binary executable native mandiri yang teroptimasi penuh (`-O3 -flto`) tanpa dependensi runtime:

```bash
# Di Linux & Android (Termux):
eas build script.eas -o program
./program

# Di Windows:
.\eas.exe build script.eas -o program.exe
.\program.exe
```

### 3. Mode Interaktif (Interactive REPL)
Jalankan `eas` tanpa argumen untuk masuk ke interactive shell:

```bash
# Di Linux & Android (Termux):
eas

# Di Windows:
.\eas.exe
```
Ketik `exit` untuk keluar dari REPL.

---

## 7. Lisensi

Proyek ini dirilis di bawah lisensi terbuka **MIT License**. Lihat berkas [`LICENSE`](LICENSE) untuk informasi lebih lanjut.

