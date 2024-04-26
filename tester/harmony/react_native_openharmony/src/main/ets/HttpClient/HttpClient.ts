import http from '@ohos.net.http';
import {
  CancelRequestCallback,
  HttpErrorResponse,
  HttpResponse,
  PartialProgress,
  RequestInterceptor,
  RequestOptions,
  ResponseInterceptor
} from './types';

export interface HttpClient {
  addResponseInterceptor(interceptor: ResponseInterceptor);

  addRequestInterceptor(interceptor: RequestInterceptor)

  sendRequest(url: string, requestOptions: RequestOptions, onProgress?: (partialProgress: PartialProgress) => void): {
    cancel: CancelRequestCallback,
    promise: Promise<HttpResponse>
  },

}

function mergeObjects<T>(baseObject: Partial<T>, overridingObject: Partial<T>): T {
  const mergedObject: Partial<Record<keyof T, any>> = {};

  const allKeys = new Set([
    ...Object.keys(baseObject),
    ...Object.keys(overridingObject)
  ]);

  allKeys.forEach((value: string) => {
    const key = value as keyof T;
    mergedObject[key] = overridingObject[key] !== null && overridingObject[key] !== undefined
      ? overridingObject[key]
      : baseObject[key];
  });

  return mergedObject as T;
}


export class DefaultHttpClient implements HttpClient {
  protected responseInterceptors: ResponseInterceptor[];
  protected requestInterceptors: RequestInterceptor[];
  protected baseRequestOptions: RequestOptions;
  private nextId: number = 0

  constructor({baseRequestOptions, responseInterceptors, requestInterceptors}: {
    baseRequestOptions?: RequestOptions
    responseInterceptors?: ResponseInterceptor[],
    requestInterceptors?: RequestInterceptor[],
  }={}) {
    this.baseRequestOptions = baseRequestOptions ?? {};
    this.responseInterceptors = responseInterceptors ?? [];
    this.requestInterceptors = requestInterceptors ?? [];
  }

  addResponseInterceptor(interceptor: ResponseInterceptor) {
    this.responseInterceptors.push(interceptor);
  }

  addRequestInterceptor(interceptor: RequestInterceptor) {
    this.requestInterceptors.push(interceptor);
  }


  sendRequest(url: string, requestOptions: RequestOptions, onProgress?: (partialProgress: PartialProgress) => void): {
    cancel: CancelRequestCallback,
    promise: Promise<HttpResponse>
  } {
    const requestId = this.createId();
    const httpRequest = http.createHttp();
    let headers: Object | undefined;
    let resultBody: ArrayBuffer;
    let responseCode: number;

    let currentLength = 0;
    let totalLength = null;
    const dataChunks: ArrayBuffer[] = [];

    const cleanup = () => {
      httpRequest.destroy();
    }

    const maybeCreateHttpResponse = () => {
      if (resultBody != undefined && headers != undefined && responseCode != undefined) {
        let response: HttpResponse = {
          statusCode: responseCode,
          headers: headers,
          body: resultBody
        }
        for (const interceptor of this.responseInterceptors) {
          if (interceptor.shouldIntercept(url, response)) {
            response = interceptor.intercept(url, response)
          }
        }

        return response
      }
    }

    let finalRequestOptions: RequestOptions = mergeObjects(this.baseRequestOptions, requestOptions);
    for (const interceptor of this.requestInterceptors) {
      if (interceptor.shouldIntercept(url, finalRequestOptions)) {
        ({ url, requestOptions } = interceptor.intercept(url, finalRequestOptions))
        finalRequestOptions = requestOptions;
      }
    }

    const promise: Promise<HttpResponse> = new Promise((resolve, reject) => {
      httpRequest.once('headersReceive', (data) => {
        headers = data;
        totalLength = headers['Content-Length'];
        let httpResponse = maybeCreateHttpResponse();
        if (httpResponse) {
          cleanup();
          resolve(httpResponse);
        }
      })

      httpRequest.on('dataReceive', (chunk) => {
        dataChunks.push(chunk);
        currentLength += chunk.byteLength;
        if (onProgress) {
          onProgress({
            totalLength: totalLength,
            lengthReceived: currentLength,
            bitsReceived: chunk,
          })
        }
      })

      httpRequest.on('dataEnd', () => {
        const totalLength = dataChunks.map(chunk => chunk.byteLength).reduce((acc, length) => acc + length, 0);
        const data = new Uint8Array(totalLength);
        let offset = 0;
        for (const chunk of dataChunks) {
          const chunkArray = new Uint8Array(chunk);
          data.set(chunkArray, offset);
          offset += chunk.byteLength;
        }
        resultBody = data.buffer;
        let httpResponse = maybeCreateHttpResponse();
        if (httpResponse) {
          cleanup();
          resolve(httpResponse);
        }
      })

      try {
        httpRequest.requestInStream(
          url,
          finalRequestOptions,
          (err, data) => {
            responseCode = data;
            if (err) {
              cleanup();
              reject({
                error: err,
                statusCode: responseCode ?? 0
              } as HttpErrorResponse);
            }
          }
        );
      } catch (err) {
        cleanup();
        reject({
          error: err,
          statusCode: 0
        } as HttpErrorResponse);
      }

    });

    const cancel = cleanup;

    return { cancel, promise };
  }


  public createId(): number {
    return this.nextId++
  }
}
