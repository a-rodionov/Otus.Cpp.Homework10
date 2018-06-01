#pragma once

#include "IStorage.h"
#include "Observable.h"

class StorageObservable : public Observable<IStorage>
{

protected:

  void Push(const std::string& data) {
    for (const auto& subscriber : subscribers) {
      auto subscriber_locked = subscriber.lock();
      if(subscriber_locked) {
        subscriber_locked->Push(data);
      }
    }
  }

  void Flush() {
    for (const auto& subscriber : subscribers) {
      auto subscriber_locked = subscriber.lock();
      if(subscriber_locked) {
        subscriber_locked->Flush();
      }
    }
  }

  void BlockStart() {
    for (const auto& subscriber : subscribers) {
      auto subscriber_locked = subscriber.lock();
      if(subscriber_locked) {
        subscriber_locked->BlockStart();
      }
    }
  }

  void BlockEnd() {
     for (const auto& subscriber : subscribers) {
      auto subscriber_locked = subscriber.lock();
      if(subscriber_locked) {
        subscriber_locked->BlockEnd();
      }
    }   
  }

};
