/**
 * wni.c
 * WordNet Import
 *
 * Import wordnet database into opencog.
 *
 * This version uses the native C progamming interfaces supplied by 
 * Princeton, as a part of the Wordnet project. 
 */

#include <wn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSZ 300

/**
 * Skip processing of colocations if this flag is set to 1
 */
static int skip_colocations = 0;

static int do_export(const char * word)
{
	if (0 == skip_colocations) return 1;
	char * p = strchr(word, '_');
	return (p == NULL);
}

static int getsspos(Synset *synp)
{
	int pos = 0;
	switch (synp->pos[0])
	{
		case 'n': pos = 1; break;
		case 'v': pos = 2; break;
		case 'a': pos = 3; break;
		case 'r': pos = 4; break;
		case 's': pos = 5; break;
		default:
			fprintf(stderr, "Error: unexpected pos %x\n", synp->pos[0]);
			exit(1);
	}
	return pos;
}

/**
 * Create a sense-key string, given a synset.
 */
static void get_sense_key(char * buff, Synset *synp, int idx)
{
	// The synp->ppos field seems to frequently contain garbage or
	// erroneous data. Have to use getsspos instead, to get the proper
	// part-of-speech.  This is OK, because hypernyms/hyponyms, etc. 
	// will always have the same part of speech (right?).  This may not
	// be true for cause-by, pertains-to, entails.
	// 
	// if (getsspos(synp) != synp->ppos[idx]) 
	//    fprintf (stderr, "fail! %d %d\n", getsspos(synp), synp->ppos[idx]);
	//
	int pos = getsspos(synp);
	if (!synp->headword)
	{
		sprintf(buff, "%s%%%d:%02d:%02d::",
		              synp->words[idx], pos,
		              synp->fnum, synp->lexid[idx]);
	}
	else
	{
		sprintf(buff, "%s%%%d:%02d:%02d:%s:%02d",
		              synp->words[idx], pos,
		              synp->fnum, synp->lexid[idx],
		              synp->headword, synp->headsense);
	}
}

/**
 * Main loop for finding a relation, and printing it out.
 * See 'man 3 wnintro' for an overview, and 'man findtheinfo'
 * for a description of how synsets are to be navigated.
 */
#define SENSE(RELNAME, BLOCK) { \
	if ((1<<RELNAME) & bitmask) \
	{ \
		Synset *nymp = findtheinfo_ds(word, pos, RELNAME, sense_num); \
		Synset *sroot = nymp; \
		if (nymp) nymp = nymp->ptrlist; \
 \
		while(nymp) \
		{ \
			if (5 != getsspos(nymp)) \
			{ \
				/* printf("<!-- gloss=%s -->\n", nymp->defn); */ \
				int i; \
				for (i=0; i<nymp->wcount; i++) \
				{ \
					if (0 == do_export(nymp->words[i])) continue; \
					get_sense_key(buff, nymp, i); \
					printf("<WordSenseNode name=\"%s\" />\n", buff); \
					(BLOCK); \
				} \
			} \
			nymp = nymp->nextss; \
		} \
		if (sroot) free_syns(sroot); \
	} \
}

/**
 * Print the relations between different synsets.
 */
static void print_nyms(char * sense_key, char * word, int sense_num, Synset *synp)
{
	char buff[BUFSZ];

	// Hmm .. GetSenseIndex() is buggy, it crashes due to bad access
	// SnsIndex *si = GetSenseIndex(sense_key);
	// printf ("nym sense=%d, %d\n", si->wnsense, sense_num);

	int pos = getsspos(synp);

	unsigned int bitmask = is_defined(word, pos);
	// printf ("mask=%x\n", bitmask);

	printf("<WordSenseNode name=\"%s\" />\n", sense_key);

	// Consult 'man 3 winintro' for details of these calls.
	//
	/* Hypernym */
	SENSE (HYPERPTR, ({
		printf("<InheritanceLink>\n");
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", sense_key);
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", buff);
		printf("</InheritanceLink>\n");
	}))

	/* Hyponym */
	SENSE (HYPOPTR, ({
		printf("<InheritanceLink>\n");
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", buff);
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", sense_key);
		printf("</InheritanceLink>\n");
	}))

	/* Similarity */
	SENSE (SIMPTR, ({
		printf("<SimilarityLink strength=\"0.8\" confidence=\"0.95\">\n");
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", sense_key);
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", buff);
		printf("</SimilarityLink>\n");
	}))

	/* Member holonym */
	SENSE (ISMEMBERPTR, ({
		printf("<HolonymLink>\n");
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", buff);
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", sense_key);
		printf("</HolonymLink>\n");
	}))

	/* Substance holonym */
	SENSE (ISSTUFFPTR, ({
		printf("<HolonymLink>\n");
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", buff);
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", sense_key);
		printf("</HolonymLink>\n");
	}))

	/* Substance holonym */
	SENSE (ISPARTPTR, ({
		printf("<HolonymLink>\n");
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", buff);
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", sense_key);
		printf("</HolonymLink>\n");
	}))

	/* Member meronym */
	SENSE (HASMEMBERPTR, ({
		printf("<HolonymLink>\n");
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", sense_key);
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", buff);
		printf("</HolonymLink>\n");
	}))

	/* Substance meronym */
	SENSE (HASSTUFFPTR, ({
		printf("<HolonymLink>\n");
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", sense_key);
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", buff);
		printf("</HolonymLink>\n");
	}))

	/* Substance meronym */
	SENSE (HASPARTPTR, ({
		printf("<HolonymLink>\n");
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", sense_key);
		printf("   <Element class=\"WordSenseNode\" name=\"%s\" />\n", buff);
		printf("</HolonymLink>\n");
	}))

	/* Some unhandled cases */
	if ((1<<ENTAILPTR) & bitmask)
	{
		fprintf(stderr, "Warning: unhandled entail for %s\n", sense_key);
	}
	if ((1<<CAUSETO) & bitmask)
	{
		fprintf(stderr, "Warning: unhandled causeto for %s\n", sense_key);
	}
	if ((1<<PPLPTR) & bitmask)
	{
		fprintf(stderr, "Warning: unhandled participle of verb for %s\n", sense_key);
	}
	if ((1<<PERTPTR) & bitmask)
	{
		fprintf(stderr, "Warning: unhandled pertaining for %s\n", sense_key);
	}
}

/**
 * Print the synset.
 * First, print the word, and associate the word-sense index to it.
 * Next, associate the part-of-speech to the word-sense index.
 * Finally, traverse the set of synset relations, and print those.
 */
static void print_synset(char * sense_key, int sense_num, Synset *synp)
{
	char * posstr = "";
	switch(synp->pos[0])
	{
		case 'n': posstr = "noun"; break;
		case 'v': posstr = "verb"; break;
		case 'a':  posstr = "adjective"; break;
		case 'r':  posstr = "adverb"; break;
		default:
			fprintf(stderr, "Error: unknown pos %x\n", synp->pos[0]);
			exit(1);
	}

	printf("<WordSenseNode name = \"%s\" />\n", sense_key);
	printf("<PartOfSpeechLink>\n");
	printf("   <Element class=\"WordSenseNode\" name = \"%s\" />\n", sense_key);
	printf("   <Element class=\"ConceptNode\" name = \"%s\" />\n", posstr);
	printf("</PartOfSpeechLink>\n");

	// Don't print gloss - some glosses have double-dash, 
	// which drives XML parser nuts.
	// printf("<!-- gloss=%s -->\n", synp->defn);

	int i;
	for (i=0; i<synp->wcount; i++)
	{
		if (0 == do_export(synp->words[i])) continue;

		printf("<WordNode name = \"%s\" />\n", synp->words[i]);
		printf("<WordSenseLink>\n");
		printf("   <Element class=\"WordNode\" name = \"%s\" />\n", synp->words[i]);
		printf("   <Element class=\"WordSenseNode\" name = \"%s\" />\n", sense_key);
		printf("</WordSenseLink>\n");

		print_nyms(sense_key, synp->words[i], sense_num, synp);
	}
}

/**
 * Parse a line out from /usr/share/wordnet/index.sense
 * The format of this line is documented in 'man index.sense'
 * Use this line to find the corresponding sysnset.
 * Print the synset, then delete the synset.
 */
static int show_index(char * index_entry)
{
	Synset *synp;

	if (!do_export(index_entry)) return -1;

	char * p = strchr(index_entry, '%') + 1;
	int ipos = atoi(p);

	char * sense_key = index_entry;
	p = strchr(index_entry, ' ');
	*p = 0;
	char * byte_offset = ++p;
	int offset = atoi(byte_offset);

	p = strchr(p, ' ') + 1;
	int sense_num = atoi(p); 

	// Read the synset corresponding to this line.
	synp = read_synset(ipos, offset, NULL);

	if (5 == ipos)
	{
		return -2;
	}

	if (!synp)
	{
		fprintf(stderr, "Error: failed to find sysnset!!\n");
		fprintf(stderr, "sense=%s pos=%d off=%d\n", sense_key, ipos, offset);
		return -3;
	}

	if (synp->hereiam != offset)
	{
		fprintf(stderr, "Error: bad offset!!\n");
		fprintf(stderr, "sense=%s pos=%d off=%d\n", sense_key, ipos, offset);
	}

	printf("data\n");
	printf("<list>\n");
	print_synset(sense_key, sense_num, synp);
	printf("</list>\n");
	printf("%c\n", 0x4);

	// free_synset() only frees one sysnet, not the whole chain of them.
	free_syns(synp);

	return 0;
}

main (int argc, char * argv[])
{
	char buff[BUFSZ];
	wninit();

	/* Default sense index location over-ridden at the command line */
	char * sense_index = "/usr/share/wordnet/index.sense";
	if (2 == argc) sense_index = argv[1];

	// open /usr/share/wordnet/index.sense
	// The format of this file is described in 'man senseidx'
	FILE *fh = fopen(sense_index, "r");
	if (!fh)
	{
		fprintf(stderr, "Fatal error: cannot open file %s\n", sense_index);
		exit(1);
	}

#ifdef TEST_STRINGS
	// Some sample strings, typical of what is encountered in
	// the index.sense file.
	strcpy(buff, "shiny%3:00:04:: 01119421 2 0");
	strcpy(buff, "abandon%2:40:01:: 02227741 2 6");
	strcpy(buff, "fast%4:02:01:: 00086000 1 16");
	strcpy(buff, "abnormal%5:00:00:immoderate:00 01533535 3 0");
	strcpy(buff, "bark%1:20:00:: 13162297 1 4");
	strcpy(buff, "abnormally%4:02:00:: 00227171 1 1");
#endif

	printf("data\n");
	printf("<list>\n");
	printf("<ConceptNode name = \"noun\" />\n");
	printf("<ConceptNode name = \"verb\" />\n");
	printf("<ConceptNode name = \"adjective\" />\n");
	printf("<ConceptNode name = \"adverb\" />\n");
	printf("</list>\n");
	printf("%c\n", 0x4);

	int cnt = 0;
	while (1)
	{
		char * p = fgets(buff, BUFSZ, fh);
		if (!p) break;

		int rc = show_index(buff);
		if (0 == rc) cnt ++;

		// printf("<!-- %d -->\n", cnt);
		if (cnt % 1000 == 0) fprintf(stderr, "Info: done processing %d word senes\n", cnt);
	}

	fprintf(stderr, "Info: finished loading %d word senses\n", cnt);
}
