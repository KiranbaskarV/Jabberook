/****************************************************************
 * 
 * 
 * 
 * 					BITBOARD CHESS ENGINE						
 * 
 * 
 * 
 * **************************************************************/

//headers

#include <stdio.h>
#include <string.h>

//define bitboard data types

#define U64 unsigned long long

// board squares

enum {
    a8, b8, c8, d8, e8, f8, g8, h8,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a1, b1, c1, d1, e1, f1, g1, h1, no_sq
};

// sides to move (which color's turn)

enum {
    white, black, both //both for representing all pieces
};

// rook or bishop choice in magic number generation

enum {
    rook, bishop
};

const char* square_to_coordinates[] = {
"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"
};

// piece bitboards (kings, knights, etc.)
U64 bitboards[12];

// occupancy bitboards (all white pieces, all black pieces, all pieces)
U64 occupancies[3];

// side to move
int side;

// holy hell
int enpassant = no_sq;

// castling possibilities representation
enum { WKC = 1, WQC = 2, BKC = 4, BQC = 8 }; // 4 bits to represent 4 independent states

// castling rights
int castle = 0;

// piece repr

enum {P, N, B, R, Q, K, p, n, b, r, q, k};

char ascii_pieces[] = "PNBRQKpnbrqk";

// convert ascii char pieces to ints

int char_pieces [] =
{
     ['P'] = P,
     ['N'] = N,
     ['B'] = B,
     ['R'] = R,
     ['Q'] = Q,
     ['K'] = K,
     ['p'] = p,
     ['n'] = n,
     ['b'] = b,
     ['r'] = r,
     ['q'] = q,
     ['k'] = k
};


/****************************************************************
 * 
 * 
 * 
 *                  RNG FOR MAGIC NUMBERS                                
 * 
 * 
 *
 * **************************************************************/

// pseudo random number state
unsigned int random_state = 1804289383;

// generate 32 bit pseudo legal numbers

unsigned int get_random_U32_number()
{
    // get current state
    unsigned int number = random_state;

    // XOR shift algorithm

    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;

    //update random number state
    random_state = number;

    return number;
}

// generate 64 bit pseudo legal numbers

U64 get_random_U64_number()
{
    // temporary variables to store 16 bits sliced values of random U32 number
    U64 n1, n2, n3, n4;

    n1 = (U64)(get_random_U32_number()) & 0xFFFF;
    n2 = (U64)(get_random_U32_number()) & 0xFFFF;
    n3 = (U64)(get_random_U32_number()) & 0xFFFF;
    n4 = (U64)(get_random_U32_number()) & 0xFFFF;

    return (n1 | n2 << 16 | n3 << 32 | n4 << 48);

}

// generate magic number candidate
U64 generate_magic_number()
{
    return get_random_U64_number() & get_random_U64_number() & get_random_U64_number();
}  


/****************************************************************
 * 
 * 
 * 
 *                  BIT MANIPULATIONS                           
 * 
 * 
 * 
 * **************************************************************/

// set/get/pop bit macros

#define get_bit(bitboard, square) ((bitboard) & (1ULL << (square)))
#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

// count bits within a bitboard

static inline int count_bits(U64 bitboard)
{
    // bit counter
    int count = 0;

    // consecutively reset LS1B

    while(bitboard)
    {
        count++;
        bitboard &= bitboard - 1;
    }

    return count;
}

// get LS1B index

static inline int get_ls1b_index(U64 bitboard)
{
    // check if bitboard is not 0
    if (bitboard)
    {
        return count_bits((bitboard & -bitboard) - 1);
    }

    else
        return -1; // return illegal index
}

/****************************************************************
 * 
 * 
 * 
 *                  INPUT AND OUTPUT                           
 * 
 * 
 * 
 * **************************************************************/


//print bitboard

void print_bitboard(U64 bitboard)
{
    for(int rank = 0; rank < 8; rank++)
    {
        //  loop over board files
        for(int file = 0; file < 8; file++)
        {
            //  convert file and rank into square index
            int square = rank * 8 + file;
            
            // print rank labels 
            if(file == 0)
                printf(" %d ", 8 - rank);

            // print state of the bit at square index
            printf(" %d", get_bit(bitboard, square) ? 1 : 0);
        }

        printf("\n");
    }

    // print file labels
    printf("\n    a b c d e f g h\n");

    // print bitboard as unsigned decimal number
    printf("\nBitboard: %llud  \n\n", bitboard);
}

void print_board()
{
    printf("\n");

    for(int rank = 0; rank < 8; rank++)
    {
        for(int file = 0; file < 8; file++)
        {
            int square = rank * 8 + file;

            // print rank label
            if(!file)
                printf(" %d ", 8 - rank);

            // piece value
            int piece = -1;

            for(int bb_piece = P; bb_piece <= k; bb_piece++)
            {
                if(get_bit(bitboards[bb_piece], square))
                    piece = bb_piece;
            }

            printf(" %c", (piece == -1) ? '.' : ascii_pieces[piece]);
        }

        printf("\n");
    }
    // print file label
    printf("\n    a b c d e f g h\n\n");

    printf("    Side:   %s\n", (side == 0) ? "white" : "black");

    printf("    Enpassant: %s\n", (enpassant != no_sq) ? square_to_coordinates[enpassant] : "no");

    printf("    Castling: %c%c%c%c\n\n", ((castle & WKC) ? 'K' : '-'),
                                         ((castle & WQC) ? 'Q' : '-'),
                                         ((castle & BKC) ? 'k' : '-'),
                                         ((castle & BQC) ? 'q' : '-'));    
}

// FEN dedug positions
#define empty_board "8/8/8/8/8/8/8/8 w - - "
#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "
#define tricky_position "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 "
#define killer_position "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1"
#define cmk_position "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 "


void parse_fen(char *fen)
{
    // clear board states

    memset(bitboards, 0ULL, sizeof(bitboards));

    memset(occupancies, 0ULL, sizeof(occupancies));

    side = white;

    enpassant = no_sq;

    castle = 0;


    for (int rank = 0; rank < 8; rank++)
    {
        for(int file = 0; file < 8; file++)
        {
            int square = rank * 8 + file;

            if((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z'))
            {
                // get the piece and set the corresponding bitboard square
                int piece = char_pieces[*fen];

                set_bit(bitboards[piece], square);

                // increment fen char pointer
                fen++;   
            }

            else if(*fen >= '0' && *fen <= '9')
            {
                int empty_squares = *fen - '0';

                // one less than empty squares as the loop increments it once anyway
                file += empty_squares - 1;

                fen++;
            }

            else if(*fen == '/')
            {
                fen++;
                //decrement file to not forward square
                file--;
            }
        }
    }
    fen++;

    side = (*fen == 'w' ? white : black);
    fen += 2;

    printf("%s \n", fen);

    if(*fen != '-')
    {
        while(*fen != ' ')
        {
            switch(*fen)
            {
                case 'K': castle |= WKC; break;
                case 'Q': castle |= WQC; break;
                case 'k': castle |= BKC; break;
                case 'q': castle |= BQC; break;
            }
            fen++;
        }
        fen++; // to skip over the space
    }

    else // ignore the dash
    {
        fen += 2;
    }
    
    printf("%s \n", fen);

    if(*fen != '-')
    {
        int f = fen[0] - 'a';
        int r = 8 - (fen[1] - '0');

        enpassant = r * 8 + f;
    }

    else enpassant = no_sq;

    // populate white occupancies
    for(int piece = P; piece <= K; piece++)
    {
        occupancies[white] |= bitboards[piece];
    }

    // populate black occupancies
    for(int piece = p; piece <= k; piece++)
    {
        occupancies[black] |= bitboards[piece];
    }

    occupancies[both] = occupancies[white] | occupancies[black];
}


/****************************************************************
 * 
 * 
 * 
 *                  ATTACK TABLES                          
 * 
 * 
 * 
 * **************************************************************/



// not_A_file const:
/*
 8  0 1 1 1 1 1 1 1
 7  0 1 1 1 1 1 1 1
 6  0 1 1 1 1 1 1 1
 5  0 1 1 1 1 1 1 1
 4  0 1 1 1 1 1 1 1
 3  0 1 1 1 1 1 1 1
 2  0 1 1 1 1 1 1 1
 1  0 1 1 1 1 1 1 1

    a b c d e f g h
*/

const U64 not_A_file = 18374403900871474942ULL;
const U64 not_H_file = 9187201950435737471ULL;
const U64 not_GH_file = 4557430888798830399ULL;
const U64 not_AB_file = 18229723555195321596ULL; 

// relevant occupancy bit counts for bishop

const int bishop_relevant_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6, 
    5, 5, 5, 5, 5, 5, 5, 5, 
    5, 5, 7, 7, 7, 7, 5, 5, 
    5, 5, 7, 9, 9, 7, 5, 5, 
    5, 5, 7, 9, 9, 7, 5, 5, 
    5, 5, 7, 7, 7, 7, 5, 5, 
    5, 5, 5, 5, 5, 5, 5, 5, 
    6, 5, 5, 5, 5, 5, 5, 6
};

// relevant occupancy bit counts for rook

const int rook_relevant_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12, 
    11, 10, 10, 10, 10, 10, 10, 11, 
    11, 10, 10, 10, 10, 10, 10, 11, 
    11, 10, 10, 10, 10, 10, 10, 11, 
    11, 10, 10, 10, 10, 10, 10, 11, 
    11, 10, 10, 10, 10, 10, 10, 11, 
    11, 10, 10, 10, 10, 10, 10, 11, 
    12, 11, 11, 11, 11, 11, 11, 12
};

// rook magic numbers

U64 rook_magic_numbers[64] =
{
0x8a80104000800020ULL,
0x140002000100040ULL,
0x2801880a0017001ULL,
0x100081001000420ULL,
0x200020010080420ULL,
0x3001c0002010008ULL,
0x8480008002000100ULL,
0x2080088004402900ULL,
0x800098204000ULL,
0x2024401000200040ULL,
0x100802000801000ULL,
0x120800800801000ULL,
0x208808088000400ULL,
0x2802200800400ULL,
0x2200800100020080ULL,
0x801000060821100ULL,
0x80044006422000ULL,
0x100808020004000ULL,
0x12108a0010204200ULL,
0x140848010000802ULL,
0x481828014002800ULL,
0x8094004002004100ULL,
0x4010040010010802ULL,
0x20008806104ULL,
0x100400080208000ULL,
0x2040002120081000ULL,
0x21200680100081ULL,
0x20100080080080ULL,
0x2000a00200410ULL,
0x20080800400ULL,
0x80088400100102ULL,
0x80004600042881ULL,
0x4040008040800020ULL,
0x440003000200801ULL,
0x4200011004500ULL,
0x188020010100100ULL,
0x14800401802800ULL,
0x2080040080800200ULL,
0x124080204001001ULL,
0x200046502000484ULL,
0x480400080088020ULL,
0x1000422010034000ULL,
0x30200100110040ULL,
0x100021010009ULL,
0x2002080100110004ULL,
0x202008004008002ULL,
0x20020004010100ULL,
0x2048440040820001ULL,
0x101002200408200ULL,
0x40802000401080ULL,
0x4008142004410100ULL,
0x2060820c0120200ULL,
0x1001004080100ULL,
0x20c020080040080ULL,
0x2935610830022400ULL,
0x44440041009200ULL,
0x280001040802101ULL,
0x2100190040002085ULL,
0x80c0084100102001ULL,
0x4024081001000421ULL,
0x20030a0244872ULL,
0x12001008414402ULL,
0x2006104900a0804ULL,
0x1004081002402ULL,
};

// bishop magic numbers

U64 bishop_magic_numbers[64] =
{
0x40040844404084ULL,
0x2004208a004208ULL,
0x10190041080202ULL,
0x108060845042010ULL,
0x581104180800210ULL,
0x2112080446200010ULL,
0x1080820820060210ULL,
0x3c0808410220200ULL,
0x4050404440404ULL,
0x21001420088ULL,
0x24d0080801082102ULL,
0x1020a0a020400ULL,
0x40308200402ULL,
0x4011002100800ULL,
0x401484104104005ULL,
0x801010402020200ULL,
0x400210c3880100ULL,
0x404022024108200ULL,
0x810018200204102ULL,
0x4002801a02003ULL,
0x85040820080400ULL,
0x810102c808880400ULL,
0xe900410884800ULL,
0x8002020480840102ULL,
0x220200865090201ULL,
0x2010100a02021202ULL,
0x152048408022401ULL,
0x20080002081110ULL,
0x4001001021004000ULL,
0x800040400a011002ULL,
0xe4004081011002ULL,
0x1c004001012080ULL,
0x8004200962a00220ULL,
0x8422100208500202ULL,
0x2000402200300c08ULL,
0x8646020080080080ULL,
0x80020a0200100808ULL,
0x2010004880111000ULL,
0x623000a080011400ULL,
0x42008c0340209202ULL,
0x209188240001000ULL,
0x400408a884001800ULL,
0x110400a6080400ULL,
0x1840060a44020800ULL,
0x90080104000041ULL,
0x201011000808101ULL,
0x1a2208080504f080ULL,
0x8012020600211212ULL,
0x500861011240000ULL,
0x180806108200800ULL,
0x4000020e01040044ULL,
0x300000261044000aULL,
0x802241102020002ULL,
0x20906061210001ULL,
0x5a84841004010310ULL,
0x4010801011c04ULL,
0xa010109502200ULL,
0x4a02012000ULL,
0x500201010098b028ULL,
0x8040002811040900ULL,
0x28000010020204ULL,
0x6000020202d0240ULL,
0x8918844842082200ULL,
0x4010011029020020ULL,
};


// pawn attacks table [side][square]

U64 pawn_attacks[2][64];

// knight attacks table [square] (one dimension as white/black knight have same attacks)

U64 knight_attacks[64];

// king attacks table [square] (one dimension just like knight)

U64 king_attacks[64];

// bishop attack masks
U64 bishop_masks[64];

// rook attack masks
U64 rook_masks[64];

// bishop attacks table [square][occupancies]
U64 bishop_attacks[64][512];

// rook attacks table [square][occupancies]
U64 rook_attacks[64][4096];

// generate pawn attacks
U64 mask_pawn_attcks(int side_to_move, int square)
{
    // result attack bitboard

    U64 attacks = 0ULL;

    // piece bitboard

    U64 bitboard = 0ULL;

    //set piece on board

    set_bit(bitboard, square); 

    //print_bitboard(bitboard);

    if(side_to_move == white) // white to move
    {
        if ((bitboard >> 7) & not_A_file) attacks |= (bitboard >> 7);
        if ((bitboard >> 9) & not_H_file) attacks |= (bitboard >> 9);
    }

    else                     // black to move
    {
        if ((bitboard << 7) & not_H_file) attacks |= (bitboard << 7);
        if ((bitboard << 9) & not_A_file) attacks |= (bitboard << 9);
    }

    return attacks;
}

// generate knight attacks

U64 mask_knight_attacks(int square)
{
    // result attack bitboard
    U64 attacks = 0ULL;

    // piece bitboard
    U64 bitboard = 0ULL;
    set_bit(bitboard, square);

    //generate knight attacks


    if ((bitboard >> 17) & not_H_file) attacks |= (bitboard >> 17);
    if ((bitboard >> 15) & not_A_file) attacks |= (bitboard >> 15);
    if ((bitboard >> 10) & not_GH_file) attacks |= (bitboard >> 10);
    if ((bitboard >> 6) & not_AB_file) attacks |= (bitboard >> 6);
    if ((bitboard << 17) & not_A_file) attacks |= (bitboard << 17);
    if ((bitboard << 15) & not_H_file) attacks |= (bitboard << 15);
    if ((bitboard << 10) & not_AB_file) attacks |= (bitboard << 10);
    if ((bitboard << 6) & not_GH_file) attacks |= (bitboard << 6);

    return attacks;
}

// generate king attacks

U64 mask_king_attacks(int square)
{
    // result attack bitboard
    U64 attacks = 0ULL;

    //piece bitboard
    U64 bitboard = 0ULL;
    set_bit(bitboard, square);

    //generate king attacks

    if(bitboard >> 8) attacks |= bitboard >> 8;
    if(bitboard << 8) attacks |= bitboard << 8;
    if((bitboard >> 1) & not_H_file) attacks |= (bitboard>>1);
    if((bitboard << 1) & not_A_file) attacks |= (bitboard<<1);
    if((bitboard >> 9) & not_H_file) attacks |= (bitboard>>9);
    if((bitboard << 7) & not_H_file) attacks |= (bitboard<<7);
    if((bitboard >> 7) & not_A_file) attacks |= (bitboard>>7);
    if((bitboard << 9) & not_A_file) attacks |= (bitboard<<9);

    return attacks;
}

// mask bishop attacks

U64 mask_bishop_attacks(int square)
{
    // result attack bitboard
    U64 attacks = 0ULL;

    // find row and col of square
    int sqr = square / 8;
    int sqf = square % 8;

    // init rank and file temps

    int r, f;

    // mask relevant bishop occupancy bits

    for(r = sqr + 1, f = sqf + 1; r <= 6 && f <= 6; r++, f++) attacks |= 1ULL << (r * 8 + f);
    for(r = sqr - 1, f = sqf + 1; r >= 1 && f <= 6; r--, f++) attacks |= 1ULL << (r * 8 + f);
    for(r = sqr + 1, f = sqf - 1; r <= 6 && f >= 1; r++, f--) attacks |= 1ULL << (r * 8 + f);
    for(r = sqr - 1, f = sqf - 1; r >= 1 && f >= 1; r--, f--) attacks |= 1ULL << (r * 8 + f);

    return attacks;
}

// mask rook attacks

U64 mask_rook_attacks(int square)
{
    // result attack bitboard
    U64 attacks = 0ULL;

    // find row and col of square
    int sqr = square / 8;
    int sqc = square % 8;

    // init rank and file temps
    int r, f;

    // mask relevant rook occupanccy bits

    for(r = sqr+1, f = sqc; r <= 6; r++) attacks |= 1ULL << (r * 8 + f);
    for(r = sqr-1, f = sqc; r >= 1; r--) attacks |= 1ULL << (r * 8 + f);
    for(r = sqr, f = sqc+1; f <= 6; f++) attacks |= 1ULL << (r * 8 + f);
    for(r = sqr, f = sqc-1; f >= 1; f--) attacks |= 1ULL << (r * 8 + f);

    return attacks;
}

// generate bishop attacks on the fly
U64 bishop_attacks_on_the_fly(int square, U64 block)
{
    // attack bitboard
    U64 attacks = 0ULL;

    // find row and col of square
    int sqr = square / 8;
    int sqf = square % 8;

    // init rank and file temps

    int r, f;

    // mask relevant bishop occupancy bits

    for(r = sqr + 1, f = sqf + 1; r <= 7 && f <= 7; r++, f++) 
    {
        attacks |= 1ULL << (r * 8 + f);
        if(block & (1ULL << (r * 8 + f))) break;
    }
    for(r = sqr - 1, f = sqf + 1; r >= 0 && f <= 7; r--, f++) 
    {
        attacks |= 1ULL << (r * 8 + f);
        if(block & (1ULL << (r * 8 + f))) break;
    }
    for(r = sqr + 1, f = sqf - 1; r <= 7 && f >= 0; r++, f--) 
    {
        attacks |= 1ULL << (r * 8 + f);
        if(block & (1ULL << (r * 8 + f))) break;
    }
    for(r = sqr - 1, f = sqf - 1; r >= 0 && f >= 0; r--, f--) 
    {
        attacks |= 1ULL << (r * 8 + f);
        if(block & (1ULL << (r * 8 + f))) break;
    }

    return attacks;
}

//generate rook attacks on the fl
U64 rook_attacks_on_the_fly(int square, U64 block)
{
    // result attack bitboard
    U64 attacks = 0ULL;

    // find row and col of square
    int sqr = square / 8;
    int sqc = square % 8;

    // init rank and file temps
    int r, f;

    // mask relevant rook occupanccy bits

    for(r = sqr+1, f = sqc; r <= 7; r++) 
    {
        attacks |= 1ULL << (r * 8 + f);
        if(block & (1ULL << (r * 8 + f))) break;
    }
    for(r = sqr-1, f = sqc; r >= 0; r--) 
    {
        attacks |= 1ULL << (r * 8 + f);
        if(block & (1ULL << (r * 8 + f))) break;
    }
    for(r = sqr, f = sqc+1; f <= 7; f++) 
    {
        attacks |= 1ULL << (r * 8 + f);
        if(block & (1ULL << (r * 8 + f))) break;
    }
    for(r = sqr, f = sqc-1; f >= 0; f--) 
    {
        attacks |= 1ULL << (r * 8 + f);
        if(block & (1ULL << (r * 8 + f))) break;
    }

    return attacks;
}

// init leaper pieces attacks
void init_leapers_attacks()
{
    // loop over 64 board squares
    for (int square = 0; square < 64; square++)
    {
        // init pawn attacks

        pawn_attacks[white][square] = mask_pawn_attcks(white, square);
        pawn_attacks[black][square] = mask_pawn_attcks(black, square);

        // init knight attacks

        knight_attacks[square] = mask_knight_attacks(square);

        //init king attacks

        king_attacks[square] = mask_king_attacks(square);
    }
}

//set occupancies

U64 set_occupancy(int occupancy_pattern_index, int bits_in_mask, U64 attack_mask)
{
    // occupancy map
    U64 occupancy = 0ULL;

    // loop over the range of bits within attack mask
    for(int count = 0; count < bits_in_mask; count++)
    {
        // get LS1B of attack mask
        int square = get_ls1b_index(attack_mask);

        // pop LS1B in attack map
        pop_bit(attack_mask, square);

        // check occupancy on board
        if(occupancy_pattern_index & (1 << count))
            // populate occupancy map
            occupancy |= (1ULL << square);
    }

    return occupancy;
}

/****************************************************************
 * 
 * 
 * 
 *                  MAGICS
 * 
 * 
 *
 * **************************************************************/

U64 find_magic_number(int square, int relevant_bits, int bishop)
{
    // init occupancies; size 4096 as max occupancy is for rook in corner
    // which has 12 occupancy bits and 4096 = 2^12

    U64 occupancies[4096];

    // init attack tables

    U64 attacks[4096];

    //init used attacks

    U64 used_attacks[4096];

    // init attack mask depending on piece

    U64 attack_mask = bishop ? mask_bishop_attacks(square) : mask_rook_attacks(square);

    // init occupancy indices - total number of possible block combinations possible
    // for slider pieces. This is just 2^relevant_bits.

    int occupancy_indices = 1 << relevant_bits;

    // loop to populate attacks table

    for (int index = 0; index < occupancy_indices; index++)
    {
        occupancies[index] = set_occupancy(index, relevant_bits, attack_mask);

        attacks[index] = bishop ? bishop_attacks_on_the_fly(square, occupancies[index]) :
                                    rook_attacks_on_the_fly(square, occupancies[index]);
    }

    // loop to test magic numbers

    for (int random_count = 0; random_count < 100000000; random_count++)
    {
        // generate magic number candidate
        U64 magic_number = generate_magic_number();

        // skip bad magic number candidates (not sure why bad; run perft later)
        if (count_bits((attack_mask * magic_number) & 0xFF00000000000000) < 6) continue;

        // initialise used attacks to 0
        memset(used_attacks, 0ULL, sizeof(used_attacks));

        // init index and fail flags
        int index, fail;

        // test the magic index for collisions
        for(index = 0, fail = 0; !fail && index < occupancy_indices; index++)
        {
            int magic_index = (int)((occupancies[index] * magic_number) >> (64 - relevant_bits));

            // if magic index works, i.e. there is no entry at this index
            if (used_attacks[magic_index] == 0ULL)
                // then init used attacks
                used_attacks[magic_index] = attacks[index];

            // else if there is a collision, i.e. different entries for same magic index
            else if (used_attacks[magic_index] != attacks[index])
                fail = 1;
        }

        // return magic number if no collisions occured
        if(!fail) return magic_number;
    }

    printf("magic number not found!\n");
    return 0ULL;

}

// init magic numbers()
void init_magic_numbers()
{
    for(int square = 0; square < 64; square++)
    {
        rook_magic_numbers[square] = find_magic_number(square, rook_relevant_bits[square], rook);   
    }

    printf("\n\n");

    for(int square = 0; square < 64; square++)
    {
        bishop_magic_numbers[square] = find_magic_number(square, bishop_relevant_bits[square], bishop);   
    }
}

void init_sliders_attacks(int bishop)
{
    for(int square = 0; square < 64; square++)
    {
        bishop_masks[square] = mask_bishop_attacks(square);
        rook_masks[square] = mask_rook_attacks(square);

        U64 attack_mask = bishop ? bishop_masks[square] : rook_masks[square];

        int relevant_bits = count_bits(attack_mask);

        int occupancy_indices = 1 << relevant_bits;

        for(int index = 0; index < occupancy_indices; index++)
        {
            if (bishop)
            {
                U64 occupancy = set_occupancy(index, relevant_bits, attack_mask);

                int magic_index = (occupancy * bishop_magic_numbers[square]) >> (64 - bishop_relevant_bits[square]);

                bishop_attacks[square][magic_index] = bishop_attacks_on_the_fly(square, occupancy);

            }

            else
            {
                U64 occupancy = set_occupancy(index, relevant_bits, attack_mask);

                int magic_index = (occupancy * rook_magic_numbers[square]) >> (64 - rook_relevant_bits[square]);

                rook_attacks[square][magic_index] = rook_attacks_on_the_fly(square, occupancy);

            }
        }
    }
}

// getting bishop attacks from a square and occupancy
static inline U64 get_bishop_attacks(int square, U64 occupancy)
{
    occupancy &= bishop_masks[square];
    occupancy *= bishop_magic_numbers[square];
    occupancy >>= 64 - bishop_relevant_bits[square];
    return bishop_attacks[square][occupancy];
}

// getting rook attacks from as square and occupancy
static inline U64 get_rook_attacks(int square, U64 occupancy)
{
    occupancy &= rook_masks[square];
    occupancy *= rook_magic_numbers[square];
    occupancy >>= 64 - rook_relevant_bits[square];
    return rook_attacks[square][occupancy];
}

static inline U64 get_queen_attacks(int square, U64 occupancy)
{
    return get_bishop_attacks(square, occupancy) |
           get_rook_attacks(square, occupancy);
}

/****************************************************************
 * 
 * 
 * 
 *                  INITIALIZATIONS                                
 * 
 * 
 *
 * **************************************************************/

void init_all()
{
    init_leapers_attacks();

    init_sliders_attacks(bishop);
    init_sliders_attacks(rook);

    //init_magic_numbers();
}

/****************************************************************
 * 
 * 
 * 
 *                  MOVE GENERATOR
 * 
 * 
 *
 * **************************************************************/

// check if given square is attacked by a given side
static inline int is_square_attacked(int square, int side)
{
    // attacked by white pawns
    if ((side == white) && pawn_attacks[black][square] & bitboards[P]) return 1;

    // attacked by black pawns
    if ((side == black) && pawn_attacks[white][square] & bitboards[p]) return 1;

    // attacked by knights
    if (knight_attacks[square] & ((side == white) ? bitboards[N] : bitboards[n])) return 1;

    // attacked by bishops
    if (get_bishop_attacks(square, occupancies[both]) & ((side == white) ? bitboards[B] : bitboards[b])) return 1;

    // attacked by rooks
    if (get_rook_attacks(square, occupancies[both]) & ((side == white) ? bitboards[R] : bitboards[r])) return 1;

    // attacked by queens
    if (get_queen_attacks(square, occupancies[both]) & ((side == white) ? bitboards[Q] : bitboards[q])) return 1;

    // attacked by kings
    if (king_attacks[square] & ((side == white) ? bitboards[K] : bitboards[k])) return 1;

    return 0;
}

void print_attacked_squares(int side)
{
    for (int rank = 0; rank < 8; rank++)
    {
        for(int file = 0; file < 8; file++)
        {
            int square = rank * 8 + file;

            if(!file)
                printf(" %d", 8 - rank);

            printf(" %d", is_square_attacked(square, side));
        }
        printf("\n");
    }

    printf("\n   a b c d e f g h\n\n");
}

static inline void generate_moves()
{
    int source_square, dest_square;

    // variables to store current piece bitboards and its attack bitboard
    U64 bitboard, attacks;

    for(int piece = P; piece <= k; piece++)
    {
        bitboard = bitboards[piece];

        // generate white pawn and white king castling moves
        if(side == white)
        {
            if (piece == P)
            {
                while(bitboard)
                {
                    source_square = get_ls1b_index(bitboard);

                    dest_square = source_square - 8;

                    // Check if destination on board and not occupied by some piece
                    if (dest_square > a8 && !get_bit(occupancies[both], dest_square))
                    {
                        // Check for promotion
                        if (source_square >= a7 && source_square <= h7)
                        {
                            printf("%s-%sn Generated pawn move\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                            printf("%s-%sb Generated pawn move\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                            printf("%s-%sr Generated pawn move\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                            printf("%s-%sq Generated pawn move\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                        }

                        // Check for double push
                        else
                        {
                            // Normal move
                            printf("%s-%s  Generated pawn move\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);

                            // Double push
                            if((source_square >= a2 && source_square <= h2) && !get_bit(occupancies[both], dest_square - 8))
                            {
                                printf("%s-%s  Generated pawn move\n", square_to_coordinates[source_square], square_to_coordinates[dest_square - 8]);
                            }
                        }
                    }

                    U64 attacks = pawn_attacks[white][source_square] & occupancies[black];

                    while(attacks)
                    {
                        dest_square = get_ls1b_index(attacks);

                        // Check if promotion after capture
                        if (source_square >= a7 && source_square <= h7)
                        {
                            printf("%sx%sn Generated pawn capture\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                            printf("%sx%sb Generated pawn capture\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                            printf("%sx%sr Generated pawn capture\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                            printf("%sx%sq Generated pawn capture\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                        }

                        else
                        {
                            // Normal capture
                            printf("%sx%s Generated pawn capture\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                        }

                        pop_bit(attacks, dest_square);
                    }

                    if (enpassant != no_sq)
                    {
                        dest_square = enpassant;

                        if(pawn_attacks[white][source_square] & (1ULL<<enpassant))
                        {
                            printf("%sx%s  Generated pawn capture (holy hell!)\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                        }
                    }

                    pop_bit(bitboard, source_square);
                }
            }

            if (piece == K)
            {
                if(castle & WKC)
                {
                    if(!get_bit(occupancies[both], f1) && !get_bit(occupancies[both], g1))
                    {
                        if(!is_square_attacked(e1, black) && !is_square_attacked(f1, black))
                        {
                            printf("e1-g1  Kingside Castle (O-O)\n");
                        }
                    }
                }

                if(castle & WQC)
                {
                    if(!get_bit(occupancies[both], d1) && !get_bit(occupancies[both], c1) && !get_bit(occupancies[both], b1))
                    {
                        if(!is_square_attacked(e1, black) && !is_square_attacked(d1, black) && !is_square_attacked(c1, black))
                        {
                            printf("e1-c1  Queenside Castle (O-O-O)\n");
                        }
                    }
                }
            }
        }

        // generate black pawn and black king castling moves
        else
        {
            if (piece == p)
            {
                 while(bitboard)
                {
                    source_square = get_ls1b_index(bitboard);

                    dest_square = source_square + 8;

                    // Check if destination on board and not occupied by some piece
                    if (dest_square < h1 && !get_bit(occupancies[both], dest_square))
                    {
                        // Check for promotion
                        if (source_square >= a2 && source_square <= h2)
                        {
                            printf("%s-%sn Generated pawn move\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                            printf("%s-%sb Generated pawn move\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                            printf("%s-%sr Generated pawn move\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                            printf("%s-%sq Generated pawn move\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                        }

                        // Check for double push
                        else
                        {
                            // Normal move
                            printf("%s-%s  Generated pawn move\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);

                            // Double push
                            if((source_square >= a7 && source_square <= h7) && !get_bit(occupancies[both], dest_square + 8))
                            {
                                printf("%s-%s  Generated pawn move\n", square_to_coordinates[source_square], square_to_coordinates[dest_square + 8]);
                            }
                        }
                    }

                    U64 attacks = pawn_attacks[black][source_square] & occupancies[white];

                    while(attacks)
                    {
                        dest_square = get_ls1b_index(attacks);

                        // Check if promotion after capture
                        if (source_square >= a2 && source_square <= h2)
                        {
                            printf("%sx%sn Generated pawn capture\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                            printf("%sx%sb Generated pawn capture\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                            printf("%sx%sr Generated pawn capture\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                            printf("%sx%sq Generated pawn capture\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                        }

                        else
                        {
                            // Normal capture
                            printf("%sx%s  Generated pawn capture\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                        }

                        pop_bit(attacks, dest_square);
                    }

                    if (enpassant != no_sq)
                    {
                        dest_square = enpassant;

                        if(pawn_attacks[black][source_square] & (1ULL<<enpassant))
                        {
                            printf("%sx%s  Generated pawn capture (holy hell!)\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                        }
                    }

                    pop_bit(bitboard, source_square);
                }
            }

            if (piece == k)
            {
                if(castle & BKC)
                {
                    if(!get_bit(occupancies[both], f8) && !get_bit(occupancies[both], g8))
                    {
                        if(!is_square_attacked(e8, white) && !is_square_attacked(f8, white))
                        {
                            printf("e8-g8  Kingside Castle (O-O)\n");
                        }
                    }
                }

                if(castle & BQC)
                {
                    if(!get_bit(occupancies[both], d8) && !get_bit(occupancies[both], c8) && !get_bit(occupancies[both], b8))
                    {
                        if(!is_square_attacked(e8, white) && !is_square_attacked(d8, white) && !is_square_attacked(c8, white))
                        {
                            printf("e8-c8  Queenside Castle (O-O-O)\n");
                        }
                    }
                }
            }
        }

        // generate knight moves

        if((side == white) ? piece == N : piece == n)
        {
            while(bitboard)
            {
                source_square = get_ls1b_index(bitboard);

                attacks = knight_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                while(attacks)
                {
                    dest_square = get_ls1b_index(attacks);

                    // quiet move
                    if(!get_bit(occupancies[both], dest_square))
                        printf("%s-%s  Knight move\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                    // capture
                    else
                        printf("%sx%s  Knight capture\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);

                    pop_bit(attacks, dest_square);
                }

                pop_bit(bitboard, source_square);
            }
        }

        // generate bishop moves

        if((side == white) ? piece == B : piece == b)
        {
            while(bitboard)
            {
                source_square = get_ls1b_index(bitboard);

                attacks = get_bishop_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                while(attacks)
                {
                    dest_square = get_ls1b_index(attacks);

                    // quiet move
                    if(!get_bit(occupancies[both], dest_square))
                        printf("%s-%s  Bishop move\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                    // capture
                    else
                        printf("%sx%s  Bishop capture\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);

                    pop_bit(attacks, dest_square);
                }

                pop_bit(bitboard, source_square);
            }
        }

        // generate rook moves

        if((side == white) ? piece == R : piece == r)
        {
            while(bitboard)
            {
                source_square = get_ls1b_index(bitboard);

                attacks = get_rook_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                while(attacks)
                {
                    dest_square = get_ls1b_index(attacks);

                    // quiet move
                    if(!get_bit(occupancies[both], dest_square))
                        printf("%s-%s  Rook move\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                    // capture
                    else
                        printf("%sx%s  Rook capture\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);

                    pop_bit(attacks, dest_square);
                }

                pop_bit(bitboard, source_square);
            }
        }

        // generate queen moves

        if((side == white) ? piece == Q : piece == q)
        {
            while(bitboard)
            {
                source_square = get_ls1b_index(bitboard);

                attacks = get_queen_attacks(source_square, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                while(attacks)
                {
                    dest_square = get_ls1b_index(attacks);

                    // quiet move
                    if(!get_bit(occupancies[both], dest_square))
                        printf("%s-%s  Queen move\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                    // capture
                    else
                        printf("%sx%s  Queen capture\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);

                    pop_bit(attacks, dest_square);
                }

                pop_bit(bitboard, source_square);
            }
        }

        // generate king moves

        if((side == white) ? piece == K : piece == k)
        {
            while(bitboard)
            {
                source_square = get_ls1b_index(bitboard);

                attacks = king_attacks[source_square] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                while(attacks)
                {
                    dest_square = get_ls1b_index(attacks);

                    // quiet move
                    if(!get_bit(occupancies[both], dest_square))
                        printf("%s-%s  King move\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);
                    // capture
                    else
                        printf("%sx%s  King capture\n", square_to_coordinates[source_square], square_to_coordinates[dest_square]);

                    pop_bit(attacks, dest_square);
                }

                pop_bit(bitboard, source_square);
            }
        }
    }
}

/****************************************************************
 * 
 * 
 * 
 * 					MAIN DRIVER									
 * 
 * 
 *
 * **************************************************************/

#include <stdlib.h>

int main() 
{

    init_all();
    parse_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 ");
    print_board();
    printf("%d \n\n", castle & WQC);
    generate_moves();

	return 0;
}