# Комментарии к результату
Реализация с использованием GLib.

```
 Usage: parselog [-d <dir>] [-p <prefix>] [-j <N>]
-d <dir> - directory with log files, default .
-p <prefix> - log files name prefix, default access.log
-j <N> - number of processing threads, default 4
```


# Задание
Написать программу, в заданное число потоков разбирающую логи веб-сервера в стандартном (“combined”) формате и
подсчитывающую агрегированную статистику: 
- общее количество отданных байт
- 10 самых “тяжёлых” по суммарному трафику URL’ов 
- 10 наиболее часто встречающихся Referer’ов.
