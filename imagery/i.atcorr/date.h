#ifndef MY_DATE_H
#define MY_DATE_H

#include "common.h"

class Date
{
	unsigned char day;
	unsigned char month;
	unsigned short year;
	
public:
	Date() { day = 0; month = 0; year = 0; }

	/* Date needs to be in the format dd/mm/yyyy to be accepted */
	Date(std::string date) 
	{
		std::istringstream buf(date);
		buf >> day;
		buf.ignore(numeric_limits<int>::max(), '/');
		buf >> month;
		buf.ignore(numeric_limits<int>::max(), '/');
		buf >> year;

		if(day > 31 || month > 12) 
			cerr << "Date expected in format dd/mm/yyyy, not as " << date << endl;
	}

	Date(std::istream& in)
	{
		in >> day;
		in.ignore(numeric_limits<int>::max(), '/');
		in >> month;
		in.ignore(numeric_limits<int>::max(), '/');
		in >> year;

		if(day > 31 || month > 12) 
			cerr << "Date expected in format dd/mm/yyyy!" << endl;
	}
	
	Date(unsigned char d, unsigned char m, unsigned short y) : day(d), month(m), year(m) {}

	unsigned char getDay() { return day; }
	unsigned char getMonth() { return month; }
	unsigned short getYear() { return year; }

	bool operator<(const Date& date)
	{
		return (year < date.year) || 
			  ((year == date.year) && (month < date.month)) ||
			  ((year == date.year) && (month == date.month) && (day < date.day));
	}
};

#endif /* MY_DATE_H */
