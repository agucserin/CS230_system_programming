## Project 1 : [Making Tiny Shell Program](https://github.com/agucserin/EE209_programming_structure/tree/main/pr1_customer_management_table)

 이 과제는 프로세스 제어와 신호 처리 개념을 익히기 위해 간단한 Unix 셸 프로그램을 작성하는 것입니다. 학생들은 기본적인 유닉스 셸 기능을 제공하는 tsh.c 파일의 주요 기능을 완성해야 합니다. 이 셸은 사용자가 명령을 입력하면 이를 파싱하고 해석하여 실행하며, 백그라운드 및 포어그라운드 작업 제어, 신호 처리(SIGINT, SIGTSTP 등), 작업 목록 관리(jobs, bg, fg, kill, export 명령) 등을 지원합니다. 셸은 사용자가 입력한 명령을 처리하고, 필요시 자식 프로세스를 포크하여 프로그램을 실행하며, 자식 프로세스의 상태 변화를 처리합니다. 과제를 통해 학생들은 유닉스 프로세스 관리, 신호 처리, 디버깅 도구 사용(GDB, OBJDUMP 등), 그리고 안전하고 효율적인 시스템 프로그래밍 능력을 기르게 됩니다.

## Project 2 : [Malloc lab - Making a Dynamic Storage Allocator](https://github.com/agucserin/EE209_programming_structure/tree/main/pr2_assembly_language_programming)

 이 과제는 C 프로그램을 위한 동적 저장소 할당기를 작성하는 것으로, malloc, free, realloc 루틴을 구현하는 것입니다. 학생들은 성능 평가를 위해 제공된 mdriver.c 프로그램을 사용하여 구현을 테스트할 수 있습니다. 할당기는 최소한의 초기화와 8바이트 정렬된 포인터 반환을 보장해야 하며, 메모리 블록의 할당, 해제, 재할당을 수행합니다. 과제는 올바른 동작과 효율적인 메모리 사용 및 처리 속도를 목표로 합니다. 학생들은 힙 일관성 검사 기능을 작성하여 할당기의 정확성을 확인하고, 성능 향상을 위해 포인터 연산을 매크로로 캡슐화하는 등의 최적화 기법을 사용할 수 있습니다. 최종 점수는 올바른 동작(20점)과 성능 지수(35점)에 따라 평가됩니다. 이 과제를 통해 학생들은 메모리 관리, 포인터 연산, 시스템 수준 프로그래밍 및 디버깅 기술을 향상시킬 수 있습니다.
