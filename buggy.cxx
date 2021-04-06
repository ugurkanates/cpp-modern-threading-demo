#include <thread>
#include <utility>
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <mutex>
#include <condition_variable>


class Word
{
private:
  std::string  data;
  int count;
public:
  Word ( std::string  data_ ) :
    data(std::move(data_))
  {}
  
  Word () :
    data((std::string )"")
  {}
  inline const std::string & getData(){
        return this->data;
  }
  inline void setData(const std::string & data_){
      this->data = data_;
  }
  inline const int & getCount() const{
        return this->count;
    }
    inline void setCount(const int & count_){
        this->count = count_;
    }
    // overload comparison so we can use with std::find()
    bool operator== (Word &rhs){
      return this->data == rhs.getData();

  }
    bool operator== (const std::string &rhs){
        return this->data == rhs ;

    }
    // thank god c++14 , c++20 for auto params ...
    // not used due some memory issues but basic idea is there
    static auto find_better(std::vector<Word*> s_wordsArray,Word * w){
        auto itre = std::find_if(s_wordsArray.begin(), s_wordsArray.end(), [&w](Word* word_to_compare){
            return *w == *word_to_compare;

        } );
        return itre;
  }

};

std::mutex g_mutex;
std::condition_variable g_cv;
bool g_ready = false;

static std::vector<Word*> s_wordsArray;
static Word s_word;
static int s_totalFound;


// Worker thread: consume words passed from the main thread and insert them
// in the 'word list' (s_wordsArray), while removing duplicates. Terminate when
// the word 'end' is encountered.
static void workerThread ()
{
  bool endEncountered = false;
  Word * w = nullptr;
  while (!endEncountered)
  {
      std::unique_lock<std::mutex> ul(g_mutex);

      // if blocked, ul.unlock() is automatically called.
      // if unblocked, ul.lock() is automatically called.
      g_cv.wait(ul, []() { return g_ready; });
      w = new Word(s_word); // Copy the word
      endEncountered = (s_word == "end") ;
      if (!endEncountered)
      {
          // Do not insert duplicate words

         auto itre = std::find_if(s_wordsArray.begin(), s_wordsArray.end(), [&w](Word* word_to_compare){
              return *w == *word_to_compare;

          } );
          //auto itre = Word::find_better(s_wordsArray,w);
          int index = 0;
          if(itre != s_wordsArray.end())
          {
              index = std::distance(s_wordsArray.begin(), itre);
              s_wordsArray.at(index)->setCount(s_wordsArray.at(index)->getCount()+1); // this could be criticial section -shared mem
              //break;
              //elem exists in the vector
          }
          else {
              w->setCount(w->getCount() + 1);
              s_wordsArray.push_back(w);
              //not exists
          }
      } // do while maybe?


      g_ready = false;
      ul.unlock();
      g_cv.notify_one();

      ul.lock();


  }
};

// Read input words from STDIN and pass them to the worker thread for
// inclusion in the word list.
// Terminate when the word 'end' has been entered.
//
static void producerThread ()
{
  bool endEncountered = false;
  std::string linebuf ; // it can be an empty string, no need to initialize it

  while (!endEncountered)
  {
      std::unique_lock<std::mutex> ul(g_mutex);
      std::cout << "enter line for the word list: " << std::endl;
      if(!std::getline(std::cin, linebuf))
          return;

      endEncountered = linebuf == "end";
      // Pass the word to the worker thread
      s_word.setData(linebuf) ;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      g_ready = true;
      ul.unlock();
      g_cv.notify_one();
      ul.lock();
      g_cv.wait(ul, []() { return g_ready == false; });

  }

}

// Repeatedly ask the user for a word and check whether it was present in the word list
// Terminate on EOF
//
static void lookupWords ()
{
  bool found = false;
  std::string linebuf;
  int index = 0;
  while(true)
  {
    std::cout << "\nEnter a word for lookup:";
    if(!std::getline(std::cin, linebuf))
          return;
    if(linebuf == "KILLME")
        break;
      // Initialize the word to search for
    Word * w = new Word();
    w->setData(linebuf) ;

    auto itr = std::find_if(s_wordsArray.begin(), s_wordsArray.end(), [&w](Word* word_to_compare){
          return *w == *word_to_compare;

      } );

    // Search for the word
    if(itr!= s_wordsArray.end())
      {
          found = true;
          index = std::distance(s_wordsArray.begin(), itr);
          //elem exists in the vector

      }

    if (found)
     {
      std::cout << "SUCCESS: " + s_wordsArray.at(index)->getData() + " was present" + \
        std::to_string(s_wordsArray.at(index)->getCount()) +  " times in the initial word list\n" << std::endl;

      ++s_totalFound;
     }
    else
      std::cout << w->getData() << " was NOT found in the initial word list" << std::endl;
  }
}

int main ()
{
  try
  {
      std::thread  worker (workerThread );
      std::thread  producer (producerThread);
      worker.join();
      producer.join();
    
    // Sort the words alphabetically
    /* sort() uses an introsort (quicksort which switches to heapsort when the recursion reaches a certain depth)
     and stable_sort() uses a merge sort. O(NlogN)  */
    std::sort( s_wordsArray.begin(), s_wordsArray.end() );

    // Print the word list
    std::cout << "\n=== Word list:" << std::endl;
    for ( auto p : s_wordsArray )
      std::cout <<  p->getData() + " number of that word is =  "+ std::to_string(p->getCount()) << std::endl;

    lookupWords();

    std::cout<< "\n=== Total words found: " + std::to_string(s_totalFound) << std::endl;
  }
  catch (std::exception & e)
  {
    std::cout<<"error:"<< e.what() <<std::endl;
  }
  
  return 0;
}