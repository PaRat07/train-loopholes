## get-inst-types.cpp
Получаем список типов, которые были подставлены в качестве параметра метода шаблонного параметра функции\
P.S.:
- функция должна быть constexpr
## gtopologically-sort-types.cpp
Функция GetSortedTypes сортирует типы в таком порядке, что их можно проинициализировать в таком порядке используя ссылки друг на друга и не возникнет такой ситуации, что потребуется ссылка на уже проинициализированный тип  