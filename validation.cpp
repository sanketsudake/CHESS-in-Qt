#include "validation.h"

validation::validation()
{
    //Nothing here
}

int validation::chooser(Tile *temp)
{
    switch(temp->pieceName)
    {
    case 'P': flag=validatePawn(temp);
              break;

    case 'R': flag=validateRook(temp);
              break;

    case 'H': flag=validateHorse(temp);
              break;

    case 'K': flag=validateKing(temp);
              break;

    case 'Q': flag=validateQueen(temp);
              break;

    case 'B': flag=validateBishop(temp);
              break;

    }

    orange();

    return flag;
}

//PAWN
int validation::validatePawn(Tile *temp)
{
    int row,col;

    row=temp->row;
    col=temp->col;
    retVal=0;

    //White Pawn
    if(temp->pieceColor)
    {
        if(row-1>=0 && !tile[row-1][col]->piece)
        {
            exp[max++]=tile[row-1][col]->tileNum;
            retVal=1;
        }

        if(row==6 && !tile[5][col]->piece && !tile[4][col]->piece)
        {
            exp[max++]=tile[row-2][col]->tileNum;
            retVal=1;
        }

        if(row-1>=0 && col-1>=0)
        {
            if(tile[row-1][col-1]->pieceColor!=temp->pieceColor && tile[row-1][col-1]->piece)
            {
            exp[max++]=tile[row-1][col-1]->tileNum;
            retVal=1;
            }
        }

        if(row-1>=0 && col+1<=7)
        {
            if(tile[row-1][col+1]->pieceColor!=temp->pieceColor && tile[row-1][col+1]->piece)
            {
                exp[max++]=tile[row-1][col+1]->tileNum;
                retVal=1;
            }
        }
    }
    else
    {
        if(row+1<=7 && !tile[row+1][col]->piece)
        {
            exp[max++]=tile[row+1][col]->tileNum;
            retVal=1;
        }

        if(row==1 && !tile[2][col]->piece && !tile[3][col]->piece)
        {
            exp[max++]=tile[row+2][col]->tileNum;
            retVal=1;
        }

        if(row+1<=7 && col-1>=0)
        {
            if(tile[row+1][col-1]->pieceColor!=temp->pieceColor && tile[row+1][col-1]->piece)
            {
                exp[max++]=tile[row+1][col-1]->tileNum;
                retVal=1;
            }
        }

        if(row+1<=7 && col+1<=7)
        {
            if(tile[row+1][col+1]->pieceColor!=temp->pieceColor && tile[row+1][col+1]->piece)
            {
                exp[max++]=tile[row+1][col+1]->tileNum;
                retVal=1;
            }
        }
    }

    return retVal;
}


//ROOK
int validation::validateRook(Tile *temp)
{
    int r,c;

    retVal=0;

    r=temp->row;
    c=temp->col;
    while(r-->0)
    {
        if(!tile[r][c]->piece)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
        }

        else if(tile[r][c]->pieceColor==temp->pieceColor)
            break;

        else if(tile[r][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
            break;
        }
    }

    r=temp->row;
    c=temp->col;
    while(r++<7)
    {
        if(!tile[r][c]->piece)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
        }

        else if(tile[r][c]->pieceColor==temp->pieceColor)
            break;

        else if(tile[r][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
            break;
        }
    }

    r=temp->row;
    c=temp->col;
    while(c++<7)
    {
        if(!tile[r][c]->piece)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
        }

        else if(tile[r][c]->pieceColor==temp->pieceColor)
            break;

        else if(tile[r][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
            break;
        }
    }

    r=temp->row;
    c=temp->col;
    while(c-->0)
    {
        if(!tile[r][c]->piece)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
        }

        else if(tile[r][c]->pieceColor==temp->pieceColor)
            break;

        else if(tile[r][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
            break;
        }
    }


    return retVal;
}


//HORSE
int validation::validateHorse(Tile *temp)
{
    int r,c;
    retVal=0;

    r=temp->row;
    c=temp->col;

    if(r-2>=0 && c-1>=0)
    {
        if(tile[r-2][c-1]->pieceColor!=temp->pieceColor || !tile[r-2][c-1]->piece)
        {
            exp[max++]=tile[r-2][c-1]->tileNum;
            retVal=1;
        }
    }

    if(r-2>=0 && c+1<=7)
    {
        if(tile[r-2][c+1]->pieceColor!=temp->pieceColor || !tile[r-2][c+1]->piece)
        {
            exp[max++]=tile[r-2][c+1]->tileNum;
            retVal=1;
        }
    }

    if(r-1>=0 && c-2>=0)
    {
        if(tile[r-1][c-2]->pieceColor!=temp->pieceColor || !tile[r-1][c-2]->piece)
        {
            exp[max++]=tile[r-1][c-2]->tileNum;
            retVal=1;
        }
    }

    if(r-1>=0 && c+2<=7)
    {
        if(tile[r-1][c+2]->pieceColor!=temp->pieceColor || !tile[r-1][c+2]->piece)
        {
            exp[max++]=tile[r-1][c+2]->tileNum;
            retVal=1;
        }
    }

    if(r+2<=7 && c+1<=7)
    {
        if(tile[r+2][c+1]->pieceColor!=temp->pieceColor || !tile[r+2][c+1]->piece)
        {
            exp[max++]=tile[r+2][c+1]->tileNum;
            retVal=1;
        }
    }

    if(r+2<=7 && c-1>=0)
    {
        if(tile[r+2][c-1]->pieceColor!=temp->pieceColor || !tile[r+2][c-1]->piece)
        {
            exp[max++]=tile[r+2][c-1]->tileNum;
            retVal=1;
        }
    }

    if(r+1<=7 && c-2>=0)
    {
        if(tile[r+1][c-2]->pieceColor!=temp->pieceColor || !tile[r+1][c-2]->piece)
        {
            exp[max++]=tile[r+1][c-2]->tileNum;
            retVal=1;
        }
    }

    if(r+1<=7 && c+2<=7)
    {
        if(tile[r+1][c+2]->pieceColor!=temp->pieceColor || !tile[r+1][c+2]->piece)
        {
            exp[max++]=tile[r+1][c+2]->tileNum;
            retVal=1;
        }
    }

    return retVal;
}


//KING
int validation::validateKing(Tile *temp)
{
    int r,c;
    retVal=0;

    r=temp->row;
    c=temp->col;

    if(r-1>=0)
    {
        if(!tile[r-1][c]->piece || tile[r-1][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r-1][c]->tileNum;
            retVal=1;
        }
    }

    if(r+1<=7)
    {
        if(!tile[r+1][c]->piece || tile[r+1][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r+1][c]->tileNum;
            retVal=1;
        }
    }

    if(c-1>=0)
    {
        if(!tile[r][c-1]->piece || tile[r][c-1]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c-1]->tileNum;
            retVal=1;
        }
    }

    if(c+1<=7)
    {
        if(!tile[r][c+1]->piece || tile[r][c+1]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c+1]->tileNum;
            retVal=1;
        }
    }

    if(r-1>=0 && c-1>=0)
    {
        if(!tile[r-1][c-1]->piece || tile[r-1][c-1]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r-1][c-1]->tileNum;
            retVal=1;
        }
    }

    if(r-1>=0 && c+1<=7)
    {
        if(!tile[r-1][c+1]->piece || tile[r-1][c+1]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r-1][c+1]->tileNum;
            retVal=1;
        }
    }

    if(r+1<=7 && c-1>=0)
    {
        if(!tile[r+1][c-1]->piece || tile[r+1][c-1]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r+1][c-1]->tileNum;
            retVal=1;
        }
    }

    if(r+1<=7 && c+1<=7)
    {
        if(!tile[r+1][c+1]->piece || tile[r+1][c+1]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r+1][c+1]->tileNum;
            retVal=1;
        }
    }

    return retVal;
}


//QUEEN
int validation::validateQueen(Tile *temp)
{
    int r,c;

    retVal=0;

    r=temp->row;
    c=temp->col;
    while(r-->0)
    {
        if(!tile[r][c]->piece)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
        }

        else if(tile[r][c]->pieceColor==temp->pieceColor)
            break;

        else if(tile[r][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
            break;
        }
    }

    r=temp->row;
    c=temp->col;
    while(r++<7)
    {
        if(!tile[r][c]->piece)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
        }

        else if(tile[r][c]->pieceColor==temp->pieceColor)
            break;

        else if(tile[r][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
            break;
        }
    }

    r=temp->row;
    c=temp->col;
    while(c++<7)
    {
        if(!tile[r][c]->piece)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
        }

        else if(tile[r][c]->pieceColor==temp->pieceColor)
            break;

        else if(tile[r][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
            break;
        }
    }

    r=temp->row;
    c=temp->col;
    while(c-->0)
    {
        if(!tile[r][c]->piece)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
        }

        else if(tile[r][c]->pieceColor==temp->pieceColor)
            break;

        else if(tile[r][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
            break;
        }
    }

    r=temp->row;
    c=temp->col;
    while(r-->0 && c++<7)
    {
        if(!tile[r][c]->piece)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
        }

        else if(tile[r][c]->pieceColor==temp->pieceColor)
            break;

        else if(tile[r][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
            break;
        }
    }

    r=temp->row;
    c=temp->col;
    while(r-->0 && c-->0)
    {
        if(!tile[r][c]->piece)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
        }

        else if(tile[r][c]->pieceColor==temp->pieceColor)
            break;

        else if(tile[r][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
            break;
        }
    }

    r=temp->row;
    c=temp->col;
    while(r++<7 && c++<7)
    {
        if(!tile[r][c]->piece)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
        }

        else if(tile[r][c]->pieceColor==temp->pieceColor)
            break;

        else if(tile[r][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
            break;
        }
    }

    r=temp->row;
    c=temp->col;
    while(r++<7 && c-->0)
    {
        if(!tile[r][c]->piece)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
        }

        else if(tile[r][c]->pieceColor==temp->pieceColor)
            break;

        else if(tile[r][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
            break;
        }
    }


    return retVal;
}

//BISHOP
int validation::validateBishop(Tile *temp)
{
    int r,c;
    retVal=0;

    r=temp->row;
    c=temp->col;
    while(r-->0 && c++<7)
    {
        if(!tile[r][c]->piece)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
        }

        else if(tile[r][c]->pieceColor==temp->pieceColor)
            break;

        else if(tile[r][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
            break;
        }
    }

    r=temp->row;
    c=temp->col;
    while(r-->0 && c-->0)
    {
        if(!tile[r][c]->piece)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
        }

        else if(tile[r][c]->pieceColor==temp->pieceColor)
            break;

        else if(tile[r][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
            break;
        }
    }

    r=temp->row;
    c=temp->col;
    while(r++<7 && c++<7)
    {
        if(!tile[r][c]->piece)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
        }

        else if(tile[r][c]->pieceColor==temp->pieceColor)
            break;

        else if(tile[r][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
            break;
        }
    }

    r=temp->row;
    c=temp->col;
    while(r++<7 && c-->0)
    {
        if(!tile[r][c]->piece)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
        }

        else if(tile[r][c]->pieceColor==temp->pieceColor)
            break;

        else if(tile[r][c]->pieceColor!=temp->pieceColor)
        {
            exp[max++]=tile[r][c]->tileNum;
            retVal=1;
            break;
        }
    }

    return retVal;
}

int validation::check(Tile *temp)
{
    int r,c,flag;
    retVal=0;

    return retVal;
}

void validation::orange()
{
    int i,n;

    for(i=0;i<max;i++)
        tile[exp[i]/8][exp[i]%8]->setStyleSheet("QLabel {background-color: orange;}");
}
