import { RNInstance } from './RNInstance';
import { HttpClient, DefaultHttpClient } from '../HttpClient/HttpClient';

export interface HttpClientProvider {
  getInstance(rnInstance: RNInstance): HttpClient;
}

export class DefaultHttpClientProvider implements HttpClientProvider {
  private static instance: HttpClient;

  public getInstance(rnInstance: RNInstance): HttpClient {
    if (!DefaultHttpClientProvider.instance) {
      DefaultHttpClientProvider.instance = new DefaultHttpClient();
    }

    return DefaultHttpClientProvider.instance;
  }
}