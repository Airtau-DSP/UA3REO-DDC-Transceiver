# UA3REO-DDC-Transceiver
DDC SDR Tranceiver project https://ua3reo.ru/

## Принцип работы

ВЧ сигнал оцифровывается высокоскоростной микросхемой АЦП, и подаётся на FPGA процессор.<br>
В нём происходит DDC/DUC преобразование (цифровое смещение частоты вниз или вверх по спектру) - по аналогии с приёмником прямого преобразования.<br>
I и Q квадратурные сигналы, полученные в ходе преобразований, поступают на микропроцессор STM32.<br>
В нём происходит фильтрация, (де)модуляция и вывод звука на аудио-кодек/USB. Также он обрабатывает весь пользовательский интерфейс.<br>
При передаче процесс происходит в обратном порядке, только в конце цепочки стоит ЦАП, преобразующий цифровой сигнал обратно в аналоговый ВЧ.<br>

Проект является учебным, в нём постоянно что-то меняется, дорабатывается. Поэтому собирать только на свой страх и риск, вы можете зря потерять свои деньги и время.<br>

## Технические характеристики

* Частота приёма: 0.5mHz - 450mHz
* Частота передачи: 0.5mHz - 25mHz
* Мощность TX: 5-8Вт
* Виды модуляции (TX/RX): CW, LSB, USB, AM, FM, WFM, DIGI
* Предусилитель на 20дБ и аттенюатор на 12дБ
* Динамический диапазон АЦП 85дБ
* Напряжение питания: 13.8в
* Потребляемый ток при приёме: ~0.35А
* Потребляемый ток при передаче: ~3.5А

## Функции трансивера

* Панорама (спектр+водопад) шириной в 48кГц
* Регулируемая полоса пропускания: нижний порог от 60гц до 500гц, верхний порог от 300гц до 15кГц
* Notch фильтр, регулируемый в полосе пропускания
* Отключаемое АРУ (AGC) с регулируемой скоростью атаки
* Карта диапазонов, с возможностью автоматического переключения моды
* Цифровое уменьшение шумов (DNR) на основе LMS фильтра
* Работа по USB, в том числе и передача звука
* CAT/PTT виртуальные COM-порты (эмуляция FT-450)
* Автоматическое управление предусилителем и аттенюатором
* CW декодер

## Сборка
Основу трансивера составляют китайские модули STM32F407VET6, EP4CE22E22C8N (можно использовать EP4CE10) и ILI9341 купленные в aliexpress. Также пригодится аудио-кодек WM8731 (Если вы не планируете использовать трансивер совместно с компьютером).<br>
Модули устанавливаются на базовую плату (её разводка, и других плат лежат в папке Scheme проекта).<br>
Также, туда устанавливаются модули АЦП (на основе AD9226) и ЦАП (на DAC904E) и перифирия вроде энкодера и динамика, все запчасти также доступны на aliexpress и в радио-магазинах.<br>

Прошивка STM32 производится через Keil или по USB шнурку в DFU Mode. Прошивка FPGA происходит через программу Quartus.
