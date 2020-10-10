// Nirvana Project
// Compile configuration parameters

#ifndef NIRVANA_CORE_CONFIG_H_
#define NIRVANA_CORE_CONFIG_H_

namespace Nirvana {
namespace Core {

// Heap parameters

/*
	HEAP_UNIT - ����������� ������ ����������� ����� ������ �� ���������.
	������ ����������� ����� ������������� � ������� ���������� �� ��� ��������.
	����� �������, �� ���� ������������, ��������� ������� ����������
	HEAP_UNIT/2 ���� �� ������ ���������� ����.
	����� ����, ������ ������� ����� ���������� 2 ���� �� HEAP_UNIT ���� ����.
	����� �������, ����������� �������� HEAP_UNIT ������� �� ���������� � ��������
	������� ���������� ������.
	� ������������ ����������� ����, ��������� ������� ���������� ������ �� ����� 8 ����
	�� ���������� ����. ��� ����������� ��������-��������������� �������� ���������� �������
	���������� ��������� ������ ������. ����� �������, ��������� ������� � �������
	���� ���������� ������.
	� ������ ���������� ������ ����� �� ��������� ����� 16 ����.
*/

const size_t HEAP_UNIT_MIN = 4;
const size_t HEAP_UNIT_DEFAULT = 16;
const size_t HEAP_UNIT_CORE = 16; // Core heap unit.
const size_t HEAP_UNIT_MAX = 4096;

/**	������ ������������ ����� ����. 
������ ������ ���� ������ ������������� ������ ������ ������ - �������������
�������� MAX (ALLOCATION_UNIT, PROTECTION_UNIT, SHARING_UNIT). ���� HEAP_DIRECTORY_SIZE
������ ���� ��������, ��������� ���� �������� ��������� ����������� ������, � ���� ����
������� �� ��������������� ���������� ������, ������ �� ������� �������� ��������.
��� Windows ������ ��������� ����� 64K. ��� ������ � �������� ��������� ALLOCATION_UNIT
� SHARING_UNIT ��� ����� ������� ������.
*/
const size_t HEAP_DIRECTORY_SIZE = 0x10000;

/** Maximum count of levels in PriorityQueue.
To provide best performance with a probabilistic time complexity of
O(logN) where N is the maximum number of elements, the queue should
have PRIORITY_QUEUE_LEVELS = logN. Too large value degrades the performance.
*/
const unsigned SYNC_DOMAIN_PRIORITY_QUEUE_LEVELS = 10; //!< For syncronization domain.
const unsigned SYS_DOMAIN_PRIORITY_QUEUE_LEVELS = 10; //!< For system-wide scheduler.

}
}

#endif
