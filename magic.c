#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "lib/board.h"
#include "lib/magic.h"
#include "lib/manager.h"

const uint64_t BISHOP_MAGICS[BOARD_SIZE] = {
  9368648609924554880ULL, 9009475591934976ULL, 4504776450605056ULL,
  1130334595844096ULL, 1725202480235520ULL, 288516396277699584ULL,
  613618303369805920ULL, 10168455467108368ULL, 9046920051966080ULL,
  36031066926022914ULL, 1152925941509587232ULL, 9301886096196101ULL,
  290536121828773904ULL, 5260205533369993472ULL, 7512287909098426400ULL,
  153141218749450240ULL, 9241386469758076456ULL, 5352528174448640064ULL,
  2310346668982272096ULL, 1154049638051909890ULL, 282645627930625ULL,
  2306405976892514304ULL, 11534281888680707074ULL, 72339630111982113ULL,
  8149474640617539202ULL, 2459884588819024896ULL, 11675583734899409218ULL,
  1196543596102144ULL, 5774635144585216ULL, 145242600416216065ULL,
  2522607328671633440ULL, 145278609400071184ULL, 5101802674455216ULL,
  650979603259904ULL, 9511646410653040801ULL, 1153493285013424640ULL,
  18016048314974752ULL, 4688397299729694976ULL,  9226754220791842050ULL,
  4611969694574863363ULL, 145532532652773378ULL, 5265289125480634376ULL,
  288239448330604544ULL, 2395019802642432ULL, 14555704381721968898ULL,
  2324459974457168384ULL, 23652833739932677ULL, 282583111844497ULL,
  4629880776036450560ULL, 5188716322066279440ULL, 146367151686549765ULL,
  1153170821083299856ULL, 2315697107408912522ULL, 2342448293961403408ULL,
  2309255902098161920ULL, 469501395595331584ULL, 4615626809856761874ULL,
  576601773662552642ULL, 621501155230386208ULL, 13835058055890469376ULL,
  3748138521932726784ULL, 9223517207018883457ULL, 9237736128969216257ULL,
  1127068154855556ULL
};

const uint64_t ROOK_MAGICS[BOARD_SIZE] = {
  612498416294952992ULL, 2377936612260610304ULL, 36037730568766080ULL,
  72075188908654856ULL, 144119655536003584ULL, 5836666216720237568ULL,
  9403535813175676288ULL, 1765412295174865024ULL, 3476919663777054752ULL,
  288300746238222339ULL, 9288811671472386ULL, 146648600474026240ULL,
  3799946587537536ULL, 704237264700928ULL, 10133167915730964ULL,
  2305983769267405952ULL, 9223634270415749248ULL, 10344480540467205ULL,
  9376496898355021824ULL, 2323998695235782656ULL, 9241527722809755650ULL,
  189159985010188292ULL, 2310421375767019786ULL, 4647717014536733827ULL,
  5585659813035147264ULL, 1442911135872321664ULL, 140814801969667ULL,
  1188959108457300100ULL, 288815318485696640ULL, 758869733499076736ULL,
  234750139167147013ULL, 2305924931420225604ULL, 9403727128727390345ULL,
  9223970239903959360ULL, 309094713112139074ULL, 38290492990967808ULL,
  3461016597114651648ULL, 181289678366835712ULL, 4927518981226496513ULL,
  1155212901905072225ULL, 36099167912755202ULL, 9024792514543648ULL,
  4611826894462124048ULL, 291045264466247688ULL, 83880127713378308ULL,
  1688867174481936ULL, 563516973121544ULL, 9227888831703941123ULL,
  703691741225216ULL, 45203259517829248ULL, 693563138976596032ULL,
  4038638777286134272ULL, 865817582546978176ULL, 13835621555058516608ULL,
  11541041685463296ULL, 288511853443695360ULL, 283749161902275ULL,
  176489098445378ULL, 2306124759338845321ULL, 720584805193941061ULL,
  4977040710267061250ULL, 10097633331715778562ULL, 325666550235288577ULL,
  1100057149646ULL
};

// uint64_t BISHOP_MAGICS[BOARD_SIZE];
// uint64_t ROOK_MAGICS[BOARD_SIZE];

const uint64_t BISHOP_SHIFTS[BOARD_SIZE] = {
  6, 5, 5, 5, 5, 5, 5, 6,
  5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5,
  6, 5, 5, 5, 5, 5, 5, 6,
};

const uint64_t ROOK_SHIFTS[BOARD_SIZE] = {
  12, 11, 11, 11, 11, 11, 11, 12,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  12, 11, 11, 11, 11, 11, 11, 12,
};

uint64_t apply(uint64_t occupancy, uint64_t magic, int shift) {
  return (occupancy * magic) >> (64 - shift);
}

uint64_t generate_rook_mask(int square) {
  uint64_t mask = 0ULL;
  int rank = square / 8;
  int file = square % 8;
  // right
  for (int f = file + 1; f <= 6; ++f) mask |= (1ULL << (rank * 8 + f));
  // left
  for (int f = file - 1; f >= 1; --f) mask |= (1ULL << (rank * 8 + f));
  // up
  for (int r = rank + 1; r <= 6; ++r) mask |= (1ULL << (r * 8 + file));
  // down
  for (int r = rank - 1; r >= 1; --r) mask |= (1ULL << (r * 8 + file));
  return mask;
}

uint64_t generate_bishop_mask(int square) {
  uint64_t mask = 0ULL;
  int rank = square / 8, file = square % 8;
  for (int r = rank + 1, f = file + 1; r <= 6 && f <= 6; ++r, ++f)
    mask |= (1ULL << (r * 8 + f));
  for (int r = rank + 1, f = file - 1; r <= 6 && f >= 1; ++r, --f)
    mask |= (1ULL << (r * 8 + f));
  for (int r = rank - 1, f = file + 1; r >= 1 && f <= 6; --r, ++f)
    mask |= (1ULL << (r * 8 + f));
  for (int r = rank - 1, f = file - 1; r >= 1 && f >= 1; --r, --f)
    mask |= (1ULL << (r * 8 + f));
  return mask;
}

void init_attack_tables(void) {
  for (int square = 0; square < 64; ++square) {
    uint64_t rmask = generate_rook_mask(square);
    uint64_t bmask = generate_bishop_mask(square);

    int rsize = 1 << ROOK_SHIFTS[square];
    int bsize = 1 << BISHOP_SHIFTS[square];

    for (int i = 0; i < rsize; ++i) rookAttacks[square][i] = 0ULL;
    for (int i = 0; i < bsize; ++i) bishopAttacks[square][i] = 0ULL;

    for (int i = 0; i < rsize; ++i) {
      uint64_t occ = set_occupancy(i, rmask);
      uint64_t idx = apply(occ, ROOK_MAGICS[square], ROOK_SHIFTS[square]);
      rookAttacks[square][idx] = compute_rook_attacks(square, occ);
    }

    for (int i = 0; i < bsize; ++i) {
      uint64_t occ = set_occupancy(i, bmask);
      uint64_t idx = apply(occ, BISHOP_MAGICS[square], BISHOP_SHIFTS[square]);
      bishopAttacks[square][idx] = compute_bishop_attacks(square, occ);
    }

    ROOK_MASKS[square] = generate_rook_mask(square);
    BISHOP_MASKS[square] = generate_bishop_mask(square);
  }
}

uint64_t set_occupancy(int index, uint64_t mask) {
  uint64_t occupancy = 0ULL;
  int bits = __builtin_popcountll(mask);
  int i;
  for (i = 0; i < bits; ++i) {
    int square = __builtin_ctzll(mask);
    mask &= mask - 1;
    if (index & (1 << i)) {
      occupancy |= (1ULL << square);
    }
  }
  return occupancy;
}

uint64_t compute_rook_attacks(int square, uint64_t occupancy) {
  uint64_t attacks = 0ULL;
  int rank = square / 8, file = square % 8;
  int r, f;

  for (r = rank + 1; r <= 7; ++r) {
    attacks |= (1ULL << (r * 8 + file));
    if (occupancy & (1ULL << (r * 8 + file))) break;
  }
  for (r = rank - 1; r >= 0; --r) {
    attacks |= (1ULL << (r * 8 + file));
    if (occupancy & (1ULL << (r * 8 + file))) break;
  }
  for (f = file + 1; f <= 7; ++f) {
    attacks |= (1ULL << (rank * 8 + f));
    if (occupancy & (1ULL << (rank * 8 + f))) break;
  }
  for (f = file - 1; f >= 0; --f) {
    attacks |= (1ULL << (rank * 8 + f));
    if (occupancy & (1ULL << (rank * 8 + f))) break;
  }

  return attacks;
}

uint64_t compute_bishop_attacks(int square, uint64_t occupancy) {
  uint64_t attacks = 0ULL;
  int rank = square / 8, file = square % 8;
  int r, f;

  for (r = rank + 1, f = file + 1; r <= 7 && f <= 7; ++r, ++f) {
    attacks |= (1ULL << (r * 8 + f));
    if (occupancy & (1ULL << (r * 8 + f))) break;
  }
  for (r = rank + 1, f = file - 1; r <= 7 && f >= 0; ++r, --f) {
    attacks |= (1ULL << (r * 8 + f));
    if (occupancy & (1ULL << (r * 8 + f))) break;
  }
  for (r = rank - 1, f = file + 1; r >= 0 && f <= 7; --r, ++f) {
    attacks |= (1ULL << (r * 8 + f));
    if (occupancy & (1ULL << (r * 8 + f))) break;
  }
  for (r = rank - 1, f = file - 1; r >= 0 && f >= 0; --r, --f) {
    attacks |= (1ULL << (r * 8 + f));
    if (occupancy & (1ULL << (r * 8 + f))) break;
  }

  return attacks;
}

uint64_t generate_rook_attacks(int square, uint64_t occupancy) {
  uint64_t index = apply((occupancy & ROOK_MASKS[square]), ROOK_MAGICS[square], ROOK_SHIFTS[square]);
  return rookAttacks[square][index];
}

uint64_t generate_bishop_attacks(int square, uint64_t occupancy) {
  uint64_t index = apply((occupancy & BISHOP_MASKS[square]), BISHOP_MAGICS[square], BISHOP_SHIFTS[square]);
  return bishopAttacks[square][index];
}

uint64_t generate_queen_attacks(int square, uint64_t occupancy) {
  return generate_rook_attacks(square, occupancy) | generate_bishop_attacks(square, occupancy);
}

void test_magic_bitboards(void) {
  init_attack_tables();
  printf("attack tables initialized.\n");
  
  printf("\n====== testing rook masks ======\n\n");
  int rsquare = 27;
  uint64_t expected = 0x0008080876080800ULL;
  uint64_t generated = generate_rook_mask(rsquare);
  
  if (expected == generated) {
    printf("rook mask passed for square %d.\n", rsquare);
  } else {
    printf("rook mask FAILED for square %d.\n", rsquare);
    printf("Expected: 0x%016llx, Got: 0x%016llx\n", expected, generated);
  }

  printf("\n====== testing bishop masks ======\n\n");
  int bsquare = 27; 
  expected = 0x0040221400142200ULL;
  generated = generate_bishop_mask(bsquare);
  
  if (generated == expected) {
    printf("bishop mask passed for square %d.\n", bsquare);
  } else {
    printf("bishop mask FAILED for square %d.\n", bsquare);
    printf("Expected: 0x%016llx, Got: 0x%016llx\n", expected, generated);
  }

  printf("\n====== testing rook attack generation ======\n\n");
  printf("occupancy: \n");
  binary_print(0x08080808e2080808ULL);
  printf("rook attacks for square %d:\n", rsquare);
  binary_print(generate_rook_attacks(27, 0x08080808e2080808ULL));
}