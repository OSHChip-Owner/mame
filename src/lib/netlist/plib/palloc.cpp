// license:GPL-2.0+
// copyright-holders:Couriersud
/*
 * palloc.c
 *
 */

#include <cstdio>

#include "pconfig.h"
#include "palloc.h"

PLIB_NAMESPACE_START()

//============================================================
//  Exceptions
//============================================================

pexception::pexception(const pstring &text)
{
	m_text = text;
	fprintf(stderr, "%s\n", m_text.cstr());
}

pmempool::pmempool(int min_alloc, int min_align)
: m_min_alloc(min_alloc), m_min_align(min_align)
{
}
pmempool::~pmempool()
{
	for (auto & b : m_blocks)
	{
		if (b.m_num_alloc != 0)
			fprintf(stderr, "Found block with dangling allocations\n");
		delete b.data;
	}
	m_blocks.clear();
}

int pmempool::new_block()
{
	block b;
	b.data = new char[m_min_alloc];
	b.cur_ptr = b.data;
	b.m_free = m_min_alloc;
	b.m_num_alloc = 0;
	m_blocks.push_back(b);
	return m_blocks.size() - 1;
}


void *pmempool::alloc(size_t size)
{
	size_t rs = (size + sizeof(info) + m_min_align - 1) & ~(m_min_align - 1);
	for (int bn=0; bn < m_blocks.size(); bn++)
	{
		auto &b = m_blocks[bn];
		if (b.m_free > rs)
		{
			b.m_free -= rs;
			b.m_num_alloc++;
			info *i = (info *) b.cur_ptr;
			i->m_block = bn;
			void *ret = (void *) (b.cur_ptr + sizeof(info));
			b.cur_ptr += rs;
			return ret;
		}
	}
	{
		int bn = new_block();
		auto &b = m_blocks[bn];
		b.m_num_alloc = 1;
		b.m_free = m_min_alloc - rs;
		info *i = (info *) b.cur_ptr;
		i->m_block = bn;
		void *ret = (void *) (b.cur_ptr + sizeof(info));
		b.cur_ptr += rs;
		return ret;
	}
}

void pmempool::free(void *ptr)
{
	char *p = (char *) ptr;

	info *i = (info *) (p - sizeof(info));
	block *b = &m_blocks[i->m_block];
	if (b->m_num_alloc == 0)
		fprintf(stderr, "Argh .. double free\n");
	else
	{
		b->m_free = m_min_alloc;
		b->cur_ptr = b->data;
	}
	b->m_num_alloc--;
}

PLIB_NAMESPACE_END()
