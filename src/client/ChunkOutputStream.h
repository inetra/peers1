/* 
 * Copyright (C) 2003-2005 RevConnect, http://www.revconnect.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

class ChunkDoneException : public Exception {
public:
	ChunkDoneException(const string& aError, int64_t aPos) throw() : Exception(aError), pos(aPos) { };
	~ChunkDoneException() { }; 
	int64_t pos;
};

class FileDoneException : public Exception {
public:
	FileDoneException(const string& aError, int64_t aPos) throw() : Exception(aError), pos(aPos) { };
	~FileDoneException() { }; 
	int64_t pos;
};

// Note: should be used before BufferedOutputStream
template<bool managed>
class ChunkOutputStream : public OutputStream
{

public:

	ChunkOutputStream(OutputStream* _os, const TTHValue* tth, int64_t _chunk) 
		: os(_os), chunk(_chunk), pos(_chunk)
    {
		fileChunks = FileChunksInfo::Get(tth);
		if(fileChunks == (FileChunksInfo*)NULL)
        {
			throw FileException("No chunks data");
		}
    }

	~ChunkOutputStream()
    {
		if(managed) delete os;
    }

	size_t write(const void* buf, size_t len) throw(Exception)
	{
		if(chunk == -1) return 0;
		if(len == 0) return 0;

    	int iRet = fileChunks->addChunkPos(chunk, pos, len);

		if(iRet == FileChunksInfo::CHUNK_LOST){
			chunk = -1;
			throw ChunkDoneException(STRING(CHUNK_OVERLAPPED), chunk);
		}

		pos += len;

		if(len > 0){
			size_t size = 0;
			
			try{
				size = os->write(buf, len);
			}catch(const Exception& e){
				fileChunks->abandonChunk(chunk);
				throw FileException(e.getError());
			}

			if(size != len){
				dcassert(0);
				throw FileException("internal error");
			}
		}

		if (iRet == FileChunksInfo::CHUNK_OVER){
			os->flush();
			chunk = -1;
			throw ChunkDoneException(CSTRING(BLOCK_FINISHED), pos);

		}else if(iRet == FileChunksInfo::FILE_OVER){
   			os->flush();
			chunk = -1;
			throw FileDoneException(Util::emptyString, pos);
		}

        return len;
    }

	size_t flush() throw(Exception) 
	{
		return os->flush();
	}

private:

	FileChunksInfo::Ptr fileChunks;
	OutputStream* os;
	int64_t chunk;
	int64_t pos;
};

