#ifndef MY_OUTPUT_H
#define MY_OUTPUT_H

#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

class Output
{
	static unsigned int pos;

public:
	/* begin a line */
	static void Begin()					
	{ 
		pos += 2; 
		fprintf(stderr, "* "); 
	}

	/* print a string */
	static void Print(std::string x)			
	{ 
		pos += x.length();
        fprintf(stderr, "%s", x.c_str());
	}

	/* print c, cnt times */
	static void Repeat(int cnt, char c) 
	{ 
		pos += cnt; 
		for(int i = 0; i < cnt; i++) fprintf(stderr, "%c", c);
	}
	
	/* end the line */


	static void End() 
	{ 
		Position(79);
        fprintf(stderr, " *\n");
		pos = 0; 
	}

	/* position the stream upto, but excluding p */
	static void Position(unsigned int p)
	{
		if(p < pos) return;
		for(unsigned int i = pos; i < p; i++)  fprintf(stderr, " ");
		pos = p - 1;
	}

	/* write a s after cnt spaces */
	static void WriteLn(int cnt, std::string s)
	{
		Begin();
		Repeat(cnt,' ');
		Print(s);
		End();
	}

	/* write a blank line */
	static void Ln()
	{
		Begin();
		End();
	}
};

#endif /* MY_OUTPUT */
